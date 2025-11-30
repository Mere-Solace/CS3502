# CS3502 - Project 3: Simple CRUD Application

A simple yet feature-rich CRUD file management GUI application built with C# WinForms. Manage files and folders with an intuitive tree-based interface, complete with context menus, state preservation, and user-friendly error handling.

## Features

- **Tree-based file browser** — Navigate directories with automatic state preservation (expanded nodes and selection persist across operations)
- **CRUD operations** — Create, read, update, and delete files and folders
- **Right-click context menu** — Quick access to operations (Open, Rename, Move, Delete, Create, Reveal in Explorer)
- **Smart delete workflow** — Choose between safe empty-only delete or recursive delete with confirmation
- **Rename & Move** — Easily rename files/folders or move them to new locations
- **Relative path display** — Shows paths relative to the root folder (e.g., `~\root\subfolder`)
- **File editor** — Built-in text editor to view and save file contents
- **Error handling** — Friendly error messages with technical logging for debugging
- **Dynamic root folder** — Choose any folder as the application root

## System Requirements

- **OS:** Windows 7 or later
- **.NET Runtime:** .NET 10.0 or compatible .NET runtime
- **Processor:** Any x86/x64 processor
- **RAM:** Minimal (< 50 MB)

## Installation & Setup

### Option 1: Build from Source (Recommended for Development)

#### Prerequisites
- [.NET SDK 10.0](https://dotnet.microsoft.com/en-us/download) or later
- Git (optional, for cloning)

#### Steps

1. **Clone or navigate to the repository:**
   ```powershell
   cd path\to\CS3502\p3-file_system
   ```

2. **Restore dependencies:**
   ```powershell
   dotnet restore
   ```

3. **Build the application:**
   ```powershell
   dotnet build
   ```

   Or for Release configuration (optimized):
   ```powershell
   dotnet build --configuration Release
   ```

4. **Run the application:**
   ```powershell
   dotnet run --project p3-file_system\p3-file_system.csproj
   ```

   Or after building, run the compiled executable directly:
   ```powershell
   .\p3-file_system\bin\Debug\net10.0\p3-file_system.exe
   ```

### Option 2: Pre-built Executable (If Available)

If a pre-built executable is provided:

1. Extract the ZIP file to a folder of your choice
2. Navigate to the extracted folder
3. Double-click `p3-file_system.exe` to launch

## Running the Application

### From the Command Line
```powershell
dotnet run --project p3-file_system\p3-file_system.csproj
```

### From Windows Explorer
Navigate to `p3-file_system\bin\Debug\net10.0\` (or `Release\` for optimized build) and double-click `p3-file_system.exe`.

### First Launch

1. The app will automatically locate or create a `root` folder at the project directory
2. You can change the root folder by clicking **"Choose Root..."** at the top of the window
3. Use the file browser on the left to navigate directories

## Usage Guide

### Creating Files & Folders

- **Create File:** Click the **"Create File"** button or right-click in the tree → **"Create File"**
  - Enter a file name in the text field at the top, then click **"Create File"**
- **Create Folder:** Click the **"Create Folder"** button or right-click in the tree → **"Create Folder"**
  - Enter a folder name when prompted

### Opening & Editing Files

- **Open a file:** Double-click it in the tree or click **"Open"** after selecting it
- **Edit content:** Modify text in the editor panel on the right
- **Save changes:** Click **"Save"** to write changes back to disk

### Renaming & Moving

- **Rename:** Right-click item → **"Rename"**, or select and use the context menu
- **Move:** Right-click item → **"Move"**, then enter the new path or just a new name
  - If you enter only a name, the item moves to the current working directory

### Deleting Files & Folders

- **Delete file:** Right-click file → **"Delete"**, then confirm
- **Delete folder:** Right-click folder → **"Delete"**, choose:
  - **Yes:** Delete only if empty (safe)
  - **No:** Delete recursively with additional confirmation (use with caution)
  - **Cancel:** Abort

### Revealing in Explorer

- Right-click any file or folder → **"Reveal in Explorer"** to open it in Windows File Explorer

### Changing the Root Folder

- Click **"Choose Root..."** at the top to select a different folder for file operations

## Project Structure

```
p3-file_system/
├── MainForm.cs              # UI orchestrator (thin layer)
├── FileSystemUtils.cs       # File I/O operations (with error handling)
├── AppConstants.cs          # Centralized configuration
├── p3-file_system.csproj    # Project file
├── appsettings.json         # Configuration file
└── Properties/
    └── launchSettings.json  # Launch configuration
```

## Architecture

- **MainForm.cs** — Handles all UI logic and user interactions; coordinates with FileSystemUtils
- **FileSystemUtils.cs** — Static utility class providing all file system operations (create, read, update, delete, rename) with wrapped exception handling
- **AppConstants.cs** — Centralized constants (root folder name, UI strings, window dimensions)
- **FileSystemException** — Custom exception for user-friendly error messages

## Error Handling

The application provides two-level error handling:
1. **User-facing:** Friendly, actionable messages in dialog boxes
2. **Developer-facing:** Technical details logged to Debug output for troubleshooting

Common issues are detected automatically:
- File locked by another process
- Insufficient disk space
- Permission denied
- Invalid file names

## Development Notes

### Building for Release

For optimized, production-ready binaries:
```powershell
dotnet build --configuration Release
dotnet publish --configuration Release --output ./publish
```

### Troubleshooting

- **"Failed to create or access root folder"** — Check file permissions or choose a different root folder
- **"The item is in use by another process"** — Close the file in other applications and try again
- **"Not enough disk space"** — Free up disk space or choose a different root folder
- **Build errors with .NET version** — Ensure you have .NET 10.0 SDK or compatible version installed

### Dependencies

- System.Windows.Forms (built-in WinForms framework)
- System.IO (file operations)
- System.Diagnostics (debug logging)

No external NuGet packages required.

## License

This project is part of CS3502 coursework.

## Contact & Support

For issues or questions, review the inline code comments in `MainForm.cs` and `FileSystemUtils.cs` for implementation details.
