
namespace Tny {

	using System;
	using System.Runtime.InteropServices;

	public class TnyException : System.Exception {
		IntPtr handle;

		public IntPtr Handle {
			get {
				return handle;				
			}
			set {
				handle = value;
			}
		}

                [DllImport("libtinymail-1.0.dll")]
		static extern IntPtr tny_error_get_message (IntPtr Handle);

                [DllImport("libtinymail-1.0.dll")]
		static extern int tny_error_get_code (IntPtr Handle);

		public override string Message {
			get {
				if (Handle != IntPtr.Zero) 
					return GLib.Marshaller.Utf8PtrToString (tny_error_get_message (Handle));
				else return "";
			} 
		}
		
		public ErrorEnum ErrorEnum {
			get {
				if (Handle != IntPtr.Zero) 
					return (ErrorEnum) tny_error_get_code (Handle);
				else 
					return ErrorEnum.NoError;
			}
		}

		public TnyException (IntPtr Handle) {
			handle = Handle;
		}

		public static TnyException New (IntPtr Handle) {
			return new TnyException (Handle);
		}
	}

	public class UnknownException : TnyException {
		public UnknownException (IntPtr Handle) : base (Handle) { }
	}

	public class MemoryException : TnyException {
		public MemoryException (IntPtr Handle) : base (Handle) { }
	}

	public class CancelException : TnyException {
		public CancelException (IntPtr Handle) : base (Handle) { }
	}

	public class IOException : TnyException {
		public IOException (IntPtr Handle) : base (Handle) { }
	}

	public class AuthenticateException : TnyException {
		public AuthenticateException (IntPtr Handle) : base (Handle) { }
	}

	public class ConnectException : TnyException {
		public ConnectException (IntPtr Handle) : base (Handle) { }
	}

	public class ServiceUnavailableException : TnyException {
		public ServiceUnavailableException (IntPtr Handle) : base (Handle) { }
	}

	public class LostConnectionException : TnyException {
		public LostConnectionException (IntPtr Handle) : base (Handle) { }
	}

	public class CertificateException : TnyException {
		public CertificateException (IntPtr Handle) : base (Handle) { }
	}

	public class FolderCreateException : TnyException {
		public FolderCreateException (IntPtr Handle) : base (Handle) { }
	}

	public class FolderRemoveException : TnyException {
		public FolderRemoveException (IntPtr Handle) : base (Handle) { }
	}

	public class FolderRenameException : TnyException {
		public FolderRenameException (IntPtr Handle) : base (Handle) { }
	}

	public class FolderIsUnknownException : TnyException {
		public FolderIsUnknownException (IntPtr Handle) : base (Handle) { }
	}

	public class ProtocolException : TnyException {
		public ProtocolException (IntPtr Handle) : base (Handle) { }
	}

	public class ActionUnsupportedException : TnyException {
		public ActionUnsupportedException (IntPtr Handle) : base (Handle) { }
	}

	public class NoSuchMessageException : TnyException {
		public NoSuchMessageException (IntPtr Handle) : base (Handle) { }
	}

	public class MessageNotAvailableException : TnyException {
		public MessageNotAvailableException (IntPtr Handle) : base (Handle) { }
	}

	public class StateException : TnyException {
		public StateException (IntPtr Handle) : base (Handle) { }
	}

	public class AddMsgException : TnyException {
		public AddMsgException (IntPtr Handle) : base (Handle) { }
	}

	public class RemoveMsgException : TnyException {
		public RemoveMsgException (IntPtr Handle) : base (Handle) { }
	}

	public class GetMsgException : TnyException {
		public GetMsgException (IntPtr Handle) : base (Handle) { }
	}

	public class SyncException : TnyException {
		public SyncException (IntPtr Handle) : base (Handle) { }
	}

	public class RefreshException : TnyException {
		public RefreshException (IntPtr Handle) : base (Handle) { }
	}

	public class CopyException : TnyException {
		public CopyException (IntPtr Handle) : base (Handle) { }
	}

	public class TransferException : TnyException {
		public TransferException (IntPtr Handle) : base (Handle) { }
	}

	public class GetFoldersException : TnyException {
		public GetFoldersException (IntPtr Handle) : base (Handle) { }
	}

	public class SendException : TnyException {
		public SendException (IntPtr Handle) : base (Handle) { }
	}

	public class MimeMalformedException : TnyException {
		public MimeMalformedException (IntPtr Handle) : base (Handle) { }
	}

	public class TnyExceptionFactory {

		[DllImport("libtinymail-1.0.dll")]
		static extern int tny_error_get_code (IntPtr Handle);

		public static TnyException Create (IntPtr Handle) {
			
			TnyException ex = null;
			ErrorEnum code = Handle != IntPtr.Zero ? (ErrorEnum) tny_error_get_code (Handle) : ErrorEnum.NoError;

			switch (code) {

			case ErrorEnum.ServiceErrorUnknown:
			ex = new UnknownException (Handle);
			break;

			case ErrorEnum.SystemErrorUnknown:
			ex = new UnknownException (Handle);
			break;

			case ErrorEnum.SystemErrorMemory:
			ex = new MemoryException (Handle);
			break;

			case ErrorEnum.SystemErrorCancel:
			ex = new CancelException (Handle);
			break;

			case ErrorEnum.IoErrorWrite:
			ex = new IOException (Handle);
			break;

			case ErrorEnum.IoErrorRead:
			ex = new IOException (Handle);
			break;

			case ErrorEnum.ServiceErrorAuthenticate:
			ex = new AuthenticateException (Handle);
			break;
			case ErrorEnum.ServiceErrorConnect:
			ex = new ConnectException (Handle);
			break;

			case ErrorEnum.ServiceErrorUnavailable:
			ex = new ServiceUnavailableException (Handle);
			break;
			case ErrorEnum.ServiceErrorLostConnection:
			ex = new LostConnectionException (Handle);
			break;

			case ErrorEnum.ServiceErrorCertificate:
			ex = new CertificateException (Handle);
			break;

			case ErrorEnum.ServiceErrorFolderCreate:
			ex = new FolderCreateException (Handle);
			break;

			case ErrorEnum.ServiceErrorFolderRemove:
			ex = new FolderRemoveException (Handle);
			break;
			case ErrorEnum.ServiceErrorFolderRename:
			ex = new FolderRenameException (Handle);
			break;

			case ErrorEnum.ServiceErrorFolderIsUnknown:
			ex = new FolderIsUnknownException (Handle);
			break;

			case ErrorEnum.ServiceErrorProtocol:
			ex = new ProtocolException (Handle);
			break;

			case ErrorEnum.ServiceErrorUnsupported:
			ex = new ActionUnsupportedException (Handle);
			break;

			case ErrorEnum.ServiceErrorNoSuchMessage:
			ex = new NoSuchMessageException (Handle);
			break;

			case ErrorEnum.ServiceErrorMessageNotAvailable:
			ex = new MessageNotAvailableException (Handle);
			break;

			case ErrorEnum.ServiceErrorState:
			ex = new StateException (Handle);
			break;

			case ErrorEnum.MimeErrorState:
			ex = new StateException (Handle);
			break;

			case ErrorEnum.ServiceErrorAddMsg:
			ex = new AddMsgException (Handle);
			break;

			case ErrorEnum.ServiceErrorRemoveMsg:
			ex = new RemoveMsgException (Handle);
			break;

			case ErrorEnum.ServiceErrorGetMsg:
			ex = new GetMsgException (Handle);
			break;

			case ErrorEnum.ServiceErrorSync:
			ex = new SyncException (Handle);
			break;

			case ErrorEnum.ServiceErrorRefresh:
			ex = new RefreshException (Handle);
			break;

			case ErrorEnum.ServiceErrorCopy:
			ex = new CopyException (Handle);
			break;

			case ErrorEnum.ServiceErrorTransfer:
			ex = new TransferException (Handle);
			break;

			case ErrorEnum.ServiceErrorGetFolders:
			ex = new GetFoldersException (Handle);
			break;

			case ErrorEnum.ServiceErrorSend:
			ex = new SendException (Handle);
			break;

			case ErrorEnum.MimeErrorMalformed:
			ex = new MimeMalformedException (Handle);
			break;

			case ErrorEnum.NoError: 
			ex = new TnyException (Handle); 
			break;

			default:
			ex = new TnyException (Handle); 
			break;

			}

			return ex;
		}
	}

}
