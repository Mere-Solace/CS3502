using System;

/// <summary>
/// Global application constants used by UI and file utilities.
/// Kept minimal and neutral so tests and UI can reference the same values.
/// </summary>
public static class AppConstants
{
    // File system
    public const string RootFolderName = "root";

    // UI strings
    public const string ErrorTitle = "Error";
    public const string ConfirmTitle = "Confirm";

    // Generic messages
    public const string MsgSelectLocation = "Select a file or folder.";
    public const string MsgEnterFileName = "Enter a file name.";

    // Limits / sizes
    public const int DefaultWindowWidth = 1200;
    public const int DefaultWindowHeight = 700;
}
