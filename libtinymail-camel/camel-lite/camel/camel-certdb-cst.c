/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jose Dapena Paz <jdapena@igalia.com>
 *
 *  Copyright 2008 Jose Dapena Paz
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <libedataserver/e-memory.h>

#include "camel-certdb.h"
#include "camel-file-utils.h"
#include "camel-private.h"

#define CAMEL_CERTDB_GET_CLASS(db)  ((CamelCertDBClass *) CAMEL_OBJECT_GET_CLASS (db))

#define CAMEL_CERTDB_VERSION  0x100

static void camel_certdb_class_init (CamelCertDBClass *klass);
static void camel_certdb_init       (CamelCertDB *certdb);
static void camel_certdb_finalize   (CamelObject *obj);

static int certdb_header_load (CamelCertDB *certdb, FILE *istream);
static int certdb_header_save (CamelCertDB *certdb, FILE *ostream);
static CamelCert *certdb_cert_load (CamelCertDB *certdb, FILE *istream);
static int certdb_cert_save (CamelCertDB *certdb, CamelCert *cert, FILE *ostream);
static CamelCert *certdb_cert_new (CamelCertDB *certdb);
static void certdb_cert_free (CamelCertDB *certdb, CamelCert *cert);

static const char *cert_get_string (CamelCertDB *certdb, CamelCert *cert, int string);
static void cert_set_string (CamelCertDB *certdb, CamelCert *cert, int string, const char *value);

static char *cert_dehexify (const char *hex_fingerprint);
static char *cert_hexify (gchar * finger);
static char *x509_dn_to_str (X509_NAME *xname);

static CamelCertTrust cert_eval_one_trust (CamelCertDB *certdb, cst_t_seqnum certID);
static CamelCertTrust cert_eval_trust (CamelCertDB *certdb, cst_t_seqnum certID);


static CamelObjectClass *parent_class = NULL;


CamelType
camel_certdb_get_type (void)
{
	static CamelType type = CAMEL_INVALID_TYPE;

	if (type == CAMEL_INVALID_TYPE) {
		type = camel_type_register (camel_object_get_type (),
					    "CamelCertDB",
					    sizeof (CamelCertDB),
					    sizeof (CamelCertDBClass),
					    (CamelObjectClassInitFunc) camel_certdb_class_init,
					    NULL,
					    (CamelObjectInitFunc) camel_certdb_init,
					    (CamelObjectFinalizeFunc) camel_certdb_finalize);
	}

	return type;
}


static void
camel_certdb_class_init (CamelCertDBClass *klass)
{
	parent_class = camel_type_get_global_classfuncs (camel_object_get_type ());

	klass->header_load = certdb_header_load;
	klass->header_save = certdb_header_save;

	klass->cert_new  = certdb_cert_new;
	klass->cert_load = certdb_cert_load;
	klass->cert_save = certdb_cert_save;
	klass->cert_free = certdb_cert_free;
	klass->cert_get_string = cert_get_string;
	klass->cert_set_string = cert_set_string;
}

static void
camel_certdb_init (CamelCertDB *certdb)
{
	certdb->priv = g_malloc0 (sizeof (struct _CamelCertDBPrivate));

	certdb->filename = NULL;
	certdb->version = CAMEL_CERTDB_VERSION;
	certdb->saved_certs = 0;

	certdb->cert_size = sizeof (CamelCert);

	certdb->cert_chunks = NULL;

	certdb->certs = NULL;
	certdb->cert_hash = NULL;

	certdb->priv->db_lock = g_mutex_new ();
	certdb->priv->io_lock = g_mutex_new ();
	certdb->priv->alloc_lock = g_mutex_new ();
	certdb->priv->ref_lock = g_mutex_new ();

	certdb->priv->cst = CST_open (FALSE, NULL);
}

static void
camel_certdb_finalize (CamelObject *obj)
{
	CamelCertDB *certdb = (CamelCertDB *) obj;
	struct _CamelCertDBPrivate *p;

	p = certdb->priv;

	g_free (certdb->filename);

	g_mutex_lock (certdb->priv->db_lock);
	CST_save (p->cst);
	CST_free (p->cst);
	g_mutex_unlock (certdb->priv->db_lock);

	g_mutex_free (p->db_lock);
	g_mutex_free (p->io_lock);
	g_mutex_free (p->alloc_lock);
	g_mutex_free (p->ref_lock);

	g_free (p);
}

/* We want to keep compatibility with the certificate format of camel, so we need
 * to convert the CST format (all the fingerprint uppercase without quotes) to the
 * one here */

static char *
cert_dehexify (const char *hex_fingerprint)
{
	GString *result = g_string_new ("");

	while (*hex_fingerprint) {
		if (*hex_fingerprint != ':') {
			result = g_string_append_c (result, g_ascii_toupper (*hex_fingerprint));
		}
			
		hex_fingerprint ++;
	}

	return g_string_free (result, FALSE);
}

static char *
cert_hexify (gchar * finger)
{
	GString *result = g_string_new ("");
	gboolean add_colon = FALSE;

	while (*finger) {
		result = g_string_append_c (result, g_ascii_tolower (*finger));
		if (add_colon && (*(finger +1)))
			result = g_string_append_c (result, ':');
		add_colon = !add_colon;
			
		finger ++;
	}

	return g_string_free (result, FALSE);
}

/* This provides a readable string for the issuer and subject fields */
static char *
x509_dn_to_str (X509_NAME *xname)
{
	gchar *result = NULL;
	BIO *bio;
	char buffer[128];

	/* FLAGS WE USE:
	 * XN_FLAG_FN_NONE (no field names)
	 * XN_FLAG_DN_REV (reverse order)
	 * XN_FLAG_SEP_CPLUS_SPC (comma and space sparated fields)
	 * ASN1_STRFLGS_RFC2253 (should help with utf8)
	 */

	bio = BIO_new (BIO_s_mem ());
	if (bio == NULL)
		return NULL;

	X509_NAME_print_ex (bio, xname, 0, 
			    ASN1_STRFLGS_RFC2253 | XN_FLAG_DN_REV | 
			    XN_FLAG_FN_NONE | XN_FLAG_SEP_CPLUS_SPC);
	BIO_gets (bio, buffer, 128);
	result = g_strdup (buffer);
	BIO_free_all (bio);

	return result;
}

/* very basic trust model implementation to fit with expected CST behavior.
 * This will add anyway certificates to manager, even valid ones. This checks if we
 * have any of the trust purposes we allow (ssl client, server and wlan), in the chain.
 *
 * NOTE: the certificate manager "email trust" is intended for SMIME, so we're using the
 * WLAN, but mainly we set the SSL CLIENT purpose.
 */
static CamelCertTrust
cert_eval_one_trust (CamelCertDB *certdb, cst_t_seqnum certID)
{
	int state;
	int purpose_valid;
	CamelCertTrust result;
	X509 *x509_cert;

	purpose_valid = (CST_is_purpose (certdb->priv->cst, certID, CST_PURPOSE_SSL_CLIENT)||
			 CST_is_purpose (certdb->priv->cst, certID, CST_PURPOSE_SSL_SERVER)||
			 CST_is_purpose (certdb->priv->cst, certID, CST_PURPOSE_SSL_WLAN));
	x509_cert = CST_get_cert (certdb->priv->cst, certID);
	state = CST_get_state (certdb->priv->cst, x509_cert);
	X509_free (x509_cert);

	if (state == CST_STATE_VALID && purpose_valid) {
		result = CAMEL_CERT_TRUST_FULLY;
	} else if (state == CST_STATE_VALID) {
		result = CAMEL_CERT_TRUST_UNKNOWN;
	} else {
		result = CAMEL_CERT_TRUST_NEVER;
	}

	return result;
}

static CamelCertTrust
cert_eval_trust (CamelCertDB *certdb, cst_t_seqnum certID)
{
	GSList *chain, *node;
	CamelCertTrust result;

	g_mutex_lock (certdb->priv->db_lock);
	result = cert_eval_one_trust (certdb, certID);
	if (result != CAMEL_CERT_TRUST_UNKNOWN) {
		g_mutex_unlock (certdb->priv->db_lock);
		return result;
	}

	chain = CST_get_chain_id_by_id (certdb->priv->cst, certID);
	node = chain;
	while (node) {
		result = cert_eval_one_trust (certdb, GPOINTER_TO_UINT (node->data));
		if (result != CAMEL_CERT_TRUST_UNKNOWN)
			break;
		node = g_slist_next (node);
	}

	g_slist_free (chain);

	g_mutex_unlock (certdb->priv->db_lock);
	return result;
}

static int 
certdb_header_load (CamelCertDB *certdb, FILE *istream)
{
	return 0;
}

static int
certdb_header_save (CamelCertDB *certdb, FILE *ostream)
{
	return 0;
}

static CamelCert *
certdb_cert_load (CamelCertDB *certdb, FILE *istream)
{
	return NULL;
}
static int
certdb_cert_save (CamelCertDB *certdb, CamelCert *cert, FILE *ostream)
{
	return 0;
}

static CamelCert *
certdb_cert_new (CamelCertDB *certdb)
{
	/* this way we set a 0 to trust level (unknown) */
	
	return g_malloc0 (certdb->cert_size);
}

CamelCertDB *
camel_certdb_new (void)
{
	return (CamelCertDB *) camel_object_new (camel_certdb_get_type ());
}


static CamelCertDB *default_certdb = NULL;
static pthread_mutex_t default_certdb_lock = PTHREAD_MUTEX_INITIALIZER;

void
camel_certdb_set_default (CamelCertDB *certdb)
{
	pthread_mutex_lock (&default_certdb_lock);

	if (default_certdb)
		camel_object_unref (default_certdb);

	if (certdb)
		camel_object_ref (certdb);

	default_certdb = certdb;

	pthread_mutex_unlock (&default_certdb_lock);
}


CamelCertDB *
camel_certdb_get_default (void)
{
	CamelCertDB *certdb;

	pthread_mutex_lock (&default_certdb_lock);

	if (default_certdb)
		camel_object_ref (default_certdb);

	certdb = default_certdb;

	pthread_mutex_unlock (&default_certdb_lock);

	return certdb;
}


void
camel_certdb_set_filename (CamelCertDB *certdb, const char *filename)
{
	g_return_if_fail (CAMEL_IS_CERTDB (certdb));
	g_return_if_fail (filename != NULL);

	CAMEL_CERTDB_LOCK (certdb, db_lock);

	g_free (certdb->filename);
	certdb->filename = g_strdup (filename);

	CAMEL_CERTDB_UNLOCK (certdb, db_lock);
}


int
camel_certdb_load (CamelCertDB *certdb)
{
	return 0;
}

int
camel_certdb_save (CamelCertDB *certdb)
{
	int result;
	g_mutex_lock (certdb->priv->db_lock);
	result = (CST_save (certdb->priv->cst) == CST_ERROR_OK);
	g_mutex_unlock (certdb->priv->db_lock);

	return result;
	
}

void
camel_certdb_touch (CamelCertDB *certdb)
{
}

CamelCert *
camel_certdb_get_cert (CamelCertDB *certdb, const char *fingerprint)
{
	CamelCert *cert = NULL;
	GSList *cst_id_list;
	X509 *x509_cert;
	gchar *dehexify;
	
	g_return_val_if_fail (CAMEL_IS_CERTDB (certdb), NULL);
	dehexify = cert_dehexify (fingerprint);

	g_mutex_lock (certdb->priv->db_lock);
	cst_id_list = CST_search_by_fingerprint (certdb->priv->cst, dehexify);
	g_free (dehexify);

	if (cst_id_list == NULL) {
		g_mutex_unlock (certdb->priv->db_lock);
		return NULL;
	}

	x509_cert = CST_get_cert (certdb->priv->cst, GPOINTER_TO_UINT (cst_id_list->data));
	if (x509_cert) {
		X509_NAME *x509_issuer;
		X509_NAME *x509_subject;
		gchar *issuer;
		gchar *subject;
		gchar *finger;
		gchar *hex_finger;
		unsigned char *der_data = NULL;
		int der_len;
			
		cert = g_malloc0 (certdb->cert_size);

		cert->certID = GPOINTER_TO_UINT (cst_id_list->data);

		x509_issuer = CST_get_issued_by_dn (x509_cert);
		issuer = x509_dn_to_str (x509_issuer);
		camel_cert_set_issuer (certdb, cert, issuer);
		g_free (issuer);
		X509_NAME_free (x509_issuer);

		x509_subject = CST_get_subject_dn (x509_cert);
		subject = x509_dn_to_str (x509_subject);
		camel_cert_set_subject (certdb, cert, subject);
		g_free (subject);
		X509_NAME_free (x509_subject);

		finger = CST_get_fingerprint (x509_cert);
		hex_finger = cert_hexify (finger);
		camel_cert_set_fingerprint (certdb, cert, hex_finger);
		g_free (finger);
		g_free (hex_finger);

		der_len = i2d_X509 (x509_cert, &der_data);
		cert->rawcert = g_byte_array_new ();
		cert->rawcert = g_byte_array_append (cert->rawcert, der_data, der_len);
		OPENSSL_free (der_data);
		X509_free (x509_cert);
		
	}
	g_slist_free (cst_id_list);
	g_mutex_unlock (certdb->priv->db_lock);

	return cert;
}

void
camel_certdb_add (CamelCertDB *certdb, CamelCert *cert)
{
	X509 *x509_cert;
	const unsigned char *buffer_p;
	GSList *old_list, *new_list, *p;
        cst_t_seqnum certID = 0;

	g_return_if_fail (CAMEL_IS_CERTDB (certdb));

	buffer_p = (const unsigned char *) cert->rawcert->data;
	x509_cert = d2i_X509 (NULL, (const unsigned char **) &buffer_p, cert->rawcert->len);

	if (x509_cert == NULL) {
		return;
	}

	g_mutex_lock (certdb->priv->db_lock);

	old_list = CST_search_by_purpose (certdb->priv->cst, CST_PURPOSE_NONE);

	CST_append_X509 (certdb->priv->cst, x509_cert);

	new_list = CST_search_by_purpose (certdb->priv->cst, CST_PURPOSE_NONE);

	for (p = new_list; p != NULL; p = g_slist_next (p)) {
		if (!g_slist_find (old_list, p->data)) {
			certID = GPOINTER_TO_UINT (p->data);
			break;
		}
	}
	g_slist_free (old_list);
	g_slist_free (new_list);

	if (certID) {
		X509_NAME *x509_issuer;
		X509_NAME *x509_subject;
		gchar *issuer;
		gchar *subject;

		CST_set_folder (certdb->priv->cst, certID, CST_FOLDER_PERSONAL);
		cert->certID = certID;

		x509_issuer = CST_get_issued_by_dn (x509_cert);
		issuer = x509_dn_to_str (x509_issuer);
		camel_cert_set_issuer (certdb, cert, issuer);
		g_free (issuer);

		x509_subject = CST_get_subject_dn (x509_cert);
		subject = x509_dn_to_str (x509_subject);
		camel_cert_set_subject (certdb, cert, subject);
		g_free (subject);

		/* we don't set any trust. This way it's "unknown" (not allowed if not set
		   as trust explicitely, or no chain member is trusting */
	}
	X509_free (x509_cert);
	g_mutex_unlock (certdb->priv->db_lock);
}

void
camel_certdb_remove (CamelCertDB *certdb, CamelCert *cert)
{
	const gchar *fingerprint;
	gchar *dehex_finger;
	GSList *cst_id_list;

	g_return_if_fail (CAMEL_IS_CERTDB (certdb));

	fingerprint = camel_cert_get_fingerprint (certdb, cert);
	if (fingerprint == NULL)
		return;
	
	dehex_finger = cert_dehexify (fingerprint);
	g_mutex_lock (certdb->priv->db_lock);
	cst_id_list = CST_search_by_fingerprint (certdb->priv->cst, dehex_finger);
	g_free (dehex_finger);

	if (cst_id_list == NULL) {
		g_mutex_unlock (certdb->priv->db_lock);
		return;
	}
	
	CST_delete_cert (certdb->priv->cst, GPOINTER_TO_UINT (cst_id_list->data));
	g_slist_free (cst_id_list);
	g_mutex_unlock (certdb->priv->db_lock);

}


CamelCert *
camel_certdb_cert_new (CamelCertDB *certdb)
{
	CamelCert *cert;

	g_return_val_if_fail (CAMEL_IS_CERTDB (certdb), NULL);

	CAMEL_CERTDB_LOCK (certdb, alloc_lock);

	cert = CAMEL_CERTDB_GET_CLASS (certdb)->cert_new (certdb);

	CAMEL_CERTDB_UNLOCK (certdb, alloc_lock);

	return cert;
}

void
camel_certdb_cert_ref (CamelCertDB *certdb, CamelCert *cert)
{
	g_return_if_fail (CAMEL_IS_CERTDB (certdb));
	g_return_if_fail (cert != NULL);

	CAMEL_CERTDB_LOCK (certdb, ref_lock);
	cert->refcount++;
	CAMEL_CERTDB_UNLOCK (certdb, ref_lock);
}

static void
certdb_cert_free (CamelCertDB *certdb, CamelCert *cert)
{
	g_free (cert->issuer);
	g_free (cert->subject);
	g_free (cert->hostname);
	g_free (cert->fingerprint);
	if (cert->rawcert)
		g_byte_array_free(cert->rawcert, TRUE);
}

void
camel_certdb_cert_unref (CamelCertDB *certdb, CamelCert *cert)
{
	g_return_if_fail (CAMEL_IS_CERTDB (certdb));
	g_return_if_fail (cert != NULL);

	CAMEL_CERTDB_LOCK (certdb, ref_lock);

	if (cert->refcount <= 1) {
		CAMEL_CERTDB_GET_CLASS (certdb)->cert_free (certdb, cert);
		if (certdb->cert_chunks)
			e_memchunk_free (certdb->cert_chunks, cert);
		else
			g_free (cert);
	} else {
		cert->refcount--;
	}

	CAMEL_CERTDB_UNLOCK (certdb, ref_lock);
}


void
camel_certdb_clear (CamelCertDB *certdb)
{
}


static const char *
cert_get_string (CamelCertDB *certdb, CamelCert *cert, int string)
{
	switch (string) {
	case CAMEL_CERT_STRING_ISSUER:
		return cert->issuer;
	case CAMEL_CERT_STRING_SUBJECT:
		return cert->subject;
	case CAMEL_CERT_STRING_HOSTNAME:
		return cert->hostname;
	case CAMEL_CERT_STRING_FINGERPRINT:
		return cert->fingerprint;
	default:
		return NULL;
	}
}


const char *
camel_cert_get_string (CamelCertDB *certdb, CamelCert *cert, int string)
{
	g_return_val_if_fail (CAMEL_IS_CERTDB (certdb), NULL);
	g_return_val_if_fail (cert != NULL, NULL);

	/* FIXME: do locking? */

	return CAMEL_CERTDB_GET_CLASS (certdb)->cert_get_string (certdb, cert, string);
}

static void
cert_set_string (CamelCertDB *certdb, CamelCert *cert, int string, const char *value)
{
	switch (string) {
	case CAMEL_CERT_STRING_ISSUER:
		g_free (cert->issuer);
		cert->issuer = g_strdup (value);
		break;
	case CAMEL_CERT_STRING_SUBJECT:
		g_free (cert->subject);
		cert->subject = g_strdup (value);
		break;
	case CAMEL_CERT_STRING_HOSTNAME:
		g_free (cert->hostname);
		cert->hostname = g_strdup (value);
		break;
	case CAMEL_CERT_STRING_FINGERPRINT:
		g_free (cert->fingerprint);
		cert->fingerprint = g_strdup (value);
		break;
	default:
		break;
	}
}


void
camel_cert_set_string (CamelCertDB *certdb, CamelCert *cert, int string, const char *value)
{
	g_return_if_fail (CAMEL_IS_CERTDB (certdb));
	g_return_if_fail (cert != NULL);

	/* FIXME: do locking? */

	CAMEL_CERTDB_GET_CLASS (certdb)->cert_set_string (certdb, cert, string, value);
}


CamelCertTrust
camel_cert_get_trust (CamelCertDB *certdb, CamelCert *cert)
{
	g_return_val_if_fail (CAMEL_IS_CERTDB (certdb), CAMEL_CERT_TRUST_UNKNOWN);
	g_return_val_if_fail (cert != NULL, CAMEL_CERT_TRUST_UNKNOWN);

	if (cert->certID) {
		return cert_eval_trust (certdb, cert->certID);
	} else {
		return cert->trust;
	}
}


void
camel_cert_set_trust (CamelCertDB *certdb, CamelCert *cert, CamelCertTrust trust)
{

	g_return_if_fail (CAMEL_IS_CERTDB (certdb));
	g_return_if_fail (cert != NULL);

	cert->trust = trust;
	g_mutex_lock (certdb->priv->db_lock);
	/* we only set the ssl client purpose. In UI this will set Browser trust, not Email trust */
	if (cert->certID) {
		switch (trust) {
		case CAMEL_CERT_TRUST_FULLY:
		case CAMEL_CERT_TRUST_ULTIMATE:
			CST_set_purpose (certdb->priv->cst, cert->certID, CST_PURPOSE_SSL_CLIENT, TRUE);
			break;
		case CAMEL_CERT_TRUST_NEVER:
			CST_set_purpose (certdb->priv->cst, cert->certID, CST_PURPOSE_SSL_CLIENT, FALSE);
			break;
		default:
			break;
		}
	}
	g_mutex_unlock (certdb->priv->db_lock);
}
