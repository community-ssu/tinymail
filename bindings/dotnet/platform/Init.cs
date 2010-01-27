
namespace Tny.Platform
{
	public class Init {

		public static void Initialize () {
			GtkSharp.LibtinymailPlatformSharp.ObjectManager.Initialize ();
			GtkSharp.LibtinymailCamelSharp.ObjectManager.Initialize ();
			// GtkSharp.LibtinymailuiGtkSharp.ObjectManager.Initialize ();
		}
	}
}

