// /home/pvanhoof/Temp/TnyNetDemo/TnyNetDemo/Main.cs created with MonoDevelop
// User: pvanhoof at 1:10 AM 2/1/2008
//
// To change standard headers go to Edit->Preferences->Coding->Standard Headers
//
// project created on 2/1/2008 at 1:10 AM
using System;
using Gtk;

namespace TnyNetDemo
{
	class MainClass
	{
		public static void Main (string[] args)
		{
			Application.Init ();
			MainWindow win = new MainWindow ();
			win.Show ();
			Application.Run ();
		}
	}
}