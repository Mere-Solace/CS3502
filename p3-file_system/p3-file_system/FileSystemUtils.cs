using System;
using System.IO;
using System.Linq;
using System.Diagnostics;

public class FileSystemException : Exception
{
    public FileSystemException(string message, Exception? inner = null) : base(message, inner) { }
}

public static class FileSystemUtils
{
    /// <summary>
    /// Returns the default root path for the application. It attempts to locate a parent
    /// folder named "p3-file_system" and uses a child folder named "root". If not found
    /// the app base directory with an appended root name is returned.
    /// </summary>
    public static string GetDefaultRoot()
    {
        try
        {
            var dir = new DirectoryInfo(Directory.GetCurrentDirectory());
            while (dir != null)
            {
                if (dir.Name.Equals("p3-file_system", StringComparison.OrdinalIgnoreCase))
                    return Path.Combine(dir.FullName, AppConstants.RootFolderName);
                dir = dir.Parent;
            }
        }
        catch { }
        return Path.Combine(AppContext.BaseDirectory, AppConstants.RootFolderName);
    }

    public static void EnsureRootExists(string rootPath)
    {
        if (string.IsNullOrEmpty(rootPath)) throw new ArgumentException("rootPath required");
        try
        {
            if (!Directory.Exists(rootPath)) Directory.CreateDirectory(rootPath);
        }
        catch (UnauthorizedAccessException ex)
        {
            throw new FileSystemException("Permission denied creating or accessing the root folder. Check your permissions.", ex);
        }
        catch (DirectoryNotFoundException ex)
        {
            throw new FileSystemException("The specified root path was not found.", ex);
        }
        catch (IOException ex)
        {
            if (IsDiskFull(ex)) throw new FileSystemException("Not enough disk space to create the root folder.", ex);
            throw new FileSystemException("I/O error while creating the root folder.", ex);
        }
        catch (Exception ex)
        {
            throw new FileSystemException("Unexpected error while ensuring root folder exists.", ex);
        }
    }

    public static string[] ListFiles(string rootPath)
    {
        try
        {
            if (!Directory.Exists(rootPath)) return Array.Empty<string>();
            return Directory.GetFiles(rootPath).OrderBy(Path.GetFileName).ToArray();
        }
        catch (UnauthorizedAccessException ex)
        {
            throw new FileSystemException("Permission denied listing files in the folder.", ex);
        }
        catch (DirectoryNotFoundException ex)
        {
            throw new FileSystemException("The folder does not exist.", ex);
        }
        catch (IOException ex)
        {
            throw new FileSystemException("I/O error while listing files.", ex);
        }
    }

    public static string ReadAllText(string filePath)
    {
        try
        {
            return File.ReadAllText(filePath);
        }
        catch (FileNotFoundException ex)
        {
            throw new FileSystemException("File not found: " + Path.GetFileName(filePath), ex);
        }
        catch (UnauthorizedAccessException ex)
        {
            throw new FileSystemException("Permission denied reading the file. Check file permissions.", ex);
        }
        catch (IOException ex)
        {
            if (IsFileLocked(ex)) throw new FileSystemException("The file is in use by another process.", ex);
            throw new FileSystemException("I/O error while reading the file.", ex);
        }
    }

    public static void WriteAllText(string filePath, string content)
    {
        // Validate file name portion for common invalid characters
        var name = Path.GetFileName(filePath);
        ValidateFileName(name);
        try
        {
            File.WriteAllText(filePath, content ?? string.Empty);
        }
        catch (UnauthorizedAccessException ex)
        {
            throw new FileSystemException("Permission denied writing the file. Check file or folder permissions.", ex);
        }
        catch (DirectoryNotFoundException ex)
        {
            throw new FileSystemException("The target folder does not exist.", ex);
        }
        catch (IOException ex)
        {
            if (IsDiskFull(ex)) throw new FileSystemException("Not enough disk space to save the file.", ex);
            if (IsFileLocked(ex)) throw new FileSystemException("The file is in use by another process.", ex);
            throw new FileSystemException("I/O error while writing the file.", ex);
        }
    }

    public static void DeleteFile(string filePath)
    {
        try
        {
            if (File.Exists(filePath)) File.Delete(filePath);
            else throw new FileNotFoundException("File not found", filePath);
        }
        catch (UnauthorizedAccessException ex)
        {
            throw new FileSystemException("Permission denied deleting the file.", ex);
        }
        catch (IOException ex)
        {
            if (IsFileLocked(ex)) throw new FileSystemException("Cannot delete the file because it is in use by another process.", ex);
            throw new FileSystemException("I/O error while deleting the file.", ex);
        }
    }

    public static void Rename(string sourcePath, string destPath)
    {
        try
        {
            if (File.Exists(destPath) || Directory.Exists(destPath))
                throw new FileSystemException("A file or folder with that name already exists.");

            if (File.Exists(sourcePath)) File.Move(sourcePath, destPath);
            else if (Directory.Exists(sourcePath)) Directory.Move(sourcePath, destPath);
            else throw new FileSystemException("Source not found.");
        }
        catch (UnauthorizedAccessException ex)
        {
            throw new FileSystemException("Permission denied while renaming.", ex);
        }
        catch (IOException ex)
        {
            if (IsFileLocked(ex)) throw new FileSystemException("The item is in use by another process.", ex);
            throw new FileSystemException("I/O error while renaming.", ex);
        }
    }

    /// <summary>
    /// Delete a directory. Throws FileSystemException with friendly messages on failure.
    /// </summary>
    public static void DeleteDirectory(string dirPath, bool recursive = true)
    {
        try
        {
            if (Directory.Exists(dirPath))
                Directory.Delete(dirPath, recursive);
            else
                throw new FileSystemException("Folder not found: " + Path.GetFileName(dirPath));
        }
        catch (UnauthorizedAccessException ex)
        {
            throw new FileSystemException("Permission denied deleting the folder.", ex);
        }
        catch (IOException ex)
        {
            if (IsFileLocked(ex)) throw new FileSystemException("The folder or items in it are in use by another process.", ex);
            throw new FileSystemException("I/O error while deleting the folder.", ex);
        }
    }

    /// <summary>
    /// Validate a file or folder name for common invalid characters and empty values.
    /// Throws FileSystemException with a friendly message when invalid.
    /// This is called early (before attempting file operations) to catch user errors quickly
    /// and avoid failures deep in I/O operations.
    /// </summary>
    public static void ValidateFileName(string? name)
    {
        if (string.IsNullOrWhiteSpace(name))
            throw new FileSystemException("Invalid name: name is empty.");
        var invalid = Path.GetInvalidFileNameChars();
        if (name.IndexOfAny(invalid) >= 0)
            throw new FileSystemException("Invalid characters in name.");
        if (name.Length > 260) // conservative limit for compatibility
            throw new FileSystemException("Name is too long.");
    }

    public static void CreateDirectory(string path)
    {
        try
        {
            if (!Directory.Exists(path)) Directory.CreateDirectory(path);
        }
        catch (UnauthorizedAccessException ex)
        {
            throw new FileSystemException("Permission denied creating the folder.", ex);
        }
        catch (IOException ex)
        {
            if (IsDiskFull(ex)) throw new FileSystemException("Not enough disk space to create the folder.", ex);
            throw new FileSystemException("I/O error while creating the folder.", ex);
        }
    }

    public static bool IsDirectory(string path) => Directory.Exists(path);
    public static bool IsFile(string path) => File.Exists(path);

    public static void RevealInExplorer(string path)
    {
        if (string.IsNullOrEmpty(path)) throw new ArgumentException("path required");
        try
        {
            Process.Start("explorer.exe", "/select,\"" + path + "\"");
        }
        catch (Exception ex)
        {
            throw new FileSystemException("Failed to open Explorer for the selected item.", ex);
        }
    }

    /// <summary>
    /// Classify whether an IOException is likely due to the file being locked by another process.
    /// Checks both the message string and the error code. Used in catch blocks to provide
    /// targeted, actionable error messages (e.g., "close the file in another app and retry").
    /// </summary>
    private static bool IsFileLocked(Exception ex)
    {
        var msg = ex.Message?.ToLowerInvariant() ?? "";
        return msg.Contains("being used by another process") || msg.Contains("used by another process") || msg.Contains("file is in use");
    }

    /// <summary>
    /// Classify whether an IOException is likely due to insufficient disk space.
    /// Checks the message string for common keywords and also checks the HResult against
    /// the Windows error code ERROR_DISK_FULL (0x80070070). This helps distinguish a disk-full
    /// error from other I/O problems so the UI can give specific guidance to the user.
    /// </summary>
    private static bool IsDiskFull(Exception ex)
    {
        var msg = ex.Message?.ToLowerInvariant() ?? "";
        if (msg.Contains("not enough space") || msg.Contains("disk full") || msg.Contains("there is not enough space")) return true;
        try
        {
            // HResult for ERROR_DISK_FULL is 0x80070070
            return ex.HResult == unchecked((int)0x80070070);
        }
        catch { return false; }
    }
}