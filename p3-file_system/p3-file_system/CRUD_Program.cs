using System;
using System.Windows.Forms;

namespace p3_file_system
{
    static class CRUD_Program
    {
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new MainForm());
        }
    }
}
