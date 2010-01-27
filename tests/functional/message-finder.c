
	TnyAccount *ac;
	int i=0;

	const gchar *URL[] = { 
"pop://philip.vanhoof%40gmail.com@pop.gmail.com/;use_ssl=wrapped/inbox/GmailIdfebedafcc22b9c6",
"pop://philip.vanhoof%40gmail.com@pop.gmail.com/;use_ssl=wrapped/inbox/GmailId112b3216c72f8e46",
"pop://philip.vanhoof%40gmail.com@pop.gmail.com/;use_ssl=wrapped/inbox/GmailId112b3216c72f8e46",
"pop://philip.vanhoof%40gmail.com@pop.gmail.com/;use_ssl=wrapped/inbox/GmailId112b3213930f6dc6",
"pop://philip.vanhoof%40gmail.com@pop.gmail.com/;use_ssl=wrapped/inbox/GmailId112b3215cf612ced",
"pop://philip.vanhoof%40gmail.com@pop.gmail.com/;use_ssl=wrapped/inbox/GmailId112b32157416f4cf" };

for (i=0; i<6; i++)
{
	ac = tny_account_store_find_account (priv->account_store, URL[i]);

	if (ac) {
		TnyFolder *fol;
		fol = tny_store_account_find_folder (ac, URL[i], NULL);
		if (fol) {
			TnyMsg *msg;
			msg = tny_folder_find_msg (fol, URL[i], NULL);
			if (msg)
				printf ("FIND 3\n");
			g_object_unref (msg);
		} else 
			printf ("FAIL 2\n");
		g_object_unref (fol);
	} else printf ("FAIL 1\n");
	g_object_unref (ac);
}
