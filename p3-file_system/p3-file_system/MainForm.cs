using System;
using System.IO;
using System.Linq;
using System.Diagnostics;
using System.Windows.Forms;

public class MainForm : Form
{
    // Application state (keeps the UI class as a thin orchestrator)
    private string rootPath;
    private string currentWorkingPath;

    // UI controls
    private Label? lblRoot;
    private Label? lblCurWorkingDirectory;
    private Button? btnChooseRoot;
    private TreeView? treeView;
    private ContextMenuStrip? treeContextMenu;
    private TextBox? txtFileName;
    private TextBox? txtContent;
    private Button? btnCreateFile;
    private Button? btnCreateFolder;
    private Button? btnOpen;
    private Button? btnSave;
    private Button? btnDelete;
    private Button? btnRefresh;

    public MainForm()
    {
        InitializeComponents();
        // Use FileSystemUtils for discovery and validation of root folder
        rootPath = FileSystemUtils.GetDefaultRoot();
        currentWorkingPath = rootPath; // initialize
        setCurrentWorkingPath(rootPath);
        try
        {
            FileSystemUtils.EnsureRootExists(rootPath);
        }
        catch (Exception ex)
        {
            ShowFriendlyError(ex, "Failed to create or access root folder.");
        }
        if (lblRoot != null) lblRoot.Text = "Root: " + rootPath;
        RefreshTree();
    }

    private void setCurrentWorkingPath(string path)
    {
        currentWorkingPath = path;
        if (lblCurWorkingDirectory != null)
        {
            // Display path relative to root folder (e.g., ~/root/subfolder instead of full path)
            string displayPath = path.Equals(rootPath, StringComparison.OrdinalIgnoreCase)
                ? "~\\root"
                : "~\\root" + (path.StartsWith(rootPath, StringComparison.OrdinalIgnoreCase)
                    ? path.Substring(rootPath.Length)
                    : "\\" + Path.GetFileName(path));
            lblCurWorkingDirectory.Text = "Current Working Directory: " + displayPath;
        }
    }

    private void InitializeComponents()
    {
        Text = "Simple File CRUD";
        Width = 1200;
        Height = 700;

        lblRoot = new Label { AutoSize = false, Height = 24, Dock = DockStyle.Top };
        btnChooseRoot = new Button { Text = "Choose Root...", Dock = DockStyle.Top, Height = 28 };
        btnChooseRoot.Click += (s, e) => ChooseRoot();

        lblCurWorkingDirectory = new Label { Dock = DockStyle.Bottom, Height = 24, Text = "Current Working Directory: " + currentWorkingPath };
        lblCurWorkingDirectory.Dock = DockStyle.Bottom;

        treeView = new TreeView { Dock = DockStyle.Left, Width = 350, ShowRootLines = true, ShowLines = true };
        treeView.NodeMouseClick += TreeView_NodeMouseClick;
        treeView.AfterSelect += TreeView_AfterSelect;
        treeView.DoubleClick += (s, e) => OpenSelectedNode();

        var rightPanel = new Panel { Dock = DockStyle.Fill };

        var topRight = new Panel { Dock = DockStyle.Top, Height = 60 };
        var lblName = new Label { Text = "File name:", AutoSize = true, Left = 6, Top = 8 };
        txtFileName = new TextBox { Left = 80, Top = 4, Width = 520, Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top };
        topRight.Controls.Add(lblName);
        topRight.Controls.Add(txtFileName);

        txtContent = new TextBox { Multiline = true, ScrollBars = ScrollBars.Both, Dock = DockStyle.Fill, AcceptsTab = true, WordWrap = false };

        var btnPanel = new FlowLayoutPanel { Dock = DockStyle.Bottom, Height = 40, FlowDirection = FlowDirection.LeftToRight, Padding = new Padding(6) };
        btnCreateFile = new Button { Text = "Create File", Width = 90 };
        btnCreateFolder = new Button { Text = "Create Folder", Width = 100 };
        btnOpen = new Button { Text = "Open", Width = 90 };
        btnSave = new Button { Text = "Save", Width = 90 };
        btnDelete = new Button { Text = "Delete", Width = 90 };
        btnRefresh = new Button { Text = "Refresh", Width = 90 };

        btnCreateFile.Click += (s, e) => CreateFile();
        btnCreateFolder.Click += (s, e) => CreateFolder();
        btnOpen.Click += (s, e) => OpenSelectedNode();
        btnSave.Click += (s, e) => SaveFile();
        btnDelete.Click += (s, e) => DeleteNode();
        btnRefresh.Click += (s, e) => RefreshTree();

        btnPanel.Controls.AddRange(new Control[] { btnCreateFile, btnCreateFolder, btnOpen, btnSave, btnDelete, btnRefresh });

        // Context menu for tree view (right-click)
        treeContextMenu = new ContextMenuStrip();
        treeContextMenu.Items.Add("Open", null, (s, e) => OpenSelectedNode());
        treeContextMenu.Items.Add("Rename", null, (s, e) => RenameSelectedNode());
        treeContextMenu.Items.Add("Create Folder", null, (s, e) => CreateFolderInNode());
        treeContextMenu.Items.Add("Create File", null, (s, e) => CreateFileInNode());
        treeContextMenu.Items.Add(new ToolStripSeparator());
        treeContextMenu.Items.Add("Delete", null, (s, e) => DeleteNode());
        treeContextMenu.Items.Add("Reveal in Explorer", null, (s, e) => RevealInExplorer());

        treeView.ContextMenuStrip = treeContextMenu;

        rightPanel.Controls.Add(lblCurWorkingDirectory);
        rightPanel.Controls.Add(txtContent);
        rightPanel.Controls.Add(topRight);
        rightPanel.Controls.Add(btnPanel);
        
        Controls.Add(rightPanel);
        Controls.Add(treeView);
        Controls.Add(btnChooseRoot);
        Controls.Add(lblRoot);
    }

    // Root discovery is provided by FileSystemUtils.GetDefaultRoot()


    private void ChooseRoot()
    {
        using (var dlg = new FolderBrowserDialog())
        {
            dlg.Description = "Choose root folder for file operations";
            dlg.SelectedPath = rootPath;
            if (dlg.ShowDialog() == DialogResult.OK)
            {
                rootPath = dlg.SelectedPath;
                try { FileSystemUtils.EnsureRootExists(rootPath); }
                catch (Exception ex) { ShowFriendlyError(ex, "Failed to create or access root folder."); }
                if (lblRoot != null) lblRoot.Text = "Root: " + rootPath;
                RefreshTree();
            }
        }
    }

    /// <summary>
    /// Rebuild the directory tree view from scratch while preserving UI state.
    /// This captures which nodes are expanded and which node is selected before clearing,
    /// then restores that state after rebuilding. This prevents the frustrating UX of
    /// having the tree collapse every time a file is created or deleted.
    /// </summary>
    private void RefreshTree()
    {
        // Preserve UI state (which nodes were expanded and the selected node)
        var expanded = CaptureExpandedNodes();
        var selected = GetSelectedPath();
        try
        {
            if (treeView == null) return;
            treeView.Nodes.Clear();
            if (!FileSystemUtils.IsDirectory(rootPath)) return;
            var rootNode = new TreeNode(Path.GetFileName(rootPath))
            {
                Tag = rootPath,
                ImageIndex = 0,
                SelectedImageIndex = 0
            };
            LoadDirectoryNode(rootNode, rootPath);
            treeView.Nodes.Add(rootNode);
            // restore expanded state
            RestoreExpandedNodes(rootNode, expanded);
            // attempt to restore selection
            if (!string.IsNullOrEmpty(selected))
            {
                var found = FindNodeByPath(treeView.Nodes, selected);
                if (found != null) treeView.SelectedNode = found;
            }
            // ensure root is visible/expanded
            rootNode.EnsureVisible();
        }
        catch (Exception ex)
        {
            ShowFriendlyError(ex, "Failed to load directory tree.");
        }
    }

    // Capture paths of expanded nodes so we can restore opened/closed state after a refresh
    /// <summary>
    /// Traverse the tree and collect full paths of all expanded nodes into a HashSet.
    /// Uses case-insensitive comparison since Windows file paths are case-insensitive.
    /// This set is later used by RestoreExpandedNodes to re-expand the same nodes
    /// after RefreshTree rebuilds the tree from disk.
    /// </summary>
    private System.Collections.Generic.HashSet<string> CaptureExpandedNodes()
    {
        var set = new System.Collections.Generic.HashSet<string>(StringComparer.OrdinalIgnoreCase);
        if (treeView == null) return set;
        foreach (TreeNode node in treeView.Nodes)
            CaptureExpandedNodesRecursive(node, set);
        return set;
    }

    private void CaptureExpandedNodesRecursive(TreeNode node, System.Collections.Generic.HashSet<string> set)
    {
        if (node.IsExpanded && node.Tag is string tag)
            set.Add(tag);
        foreach (TreeNode child in node.Nodes)
            CaptureExpandedNodesRecursive(child, set);
    }

    /// <summary>
    /// Recursively walk the rebuilt tree and expand nodes whose paths are in the expandedPaths set.
    /// Wrapped in try/catch because Expand() can fail silently if node is not yet fully initialized.
    /// This is called after LoadDirectoryNode completes so all nodes exist in memory.
    /// </summary>
    private void RestoreExpandedNodes(TreeNode node, System.Collections.Generic.HashSet<string> expandedPaths)
    {
        if (node.Tag is string tag && expandedPaths.Contains(tag))
        {
            try { node.Expand(); } catch { }
        }
        foreach (TreeNode child in node.Nodes)
            RestoreExpandedNodes(child, expandedPaths);
    }

    /// <summary>
    /// Recursively search the tree for a node whose Tag matches the given path (case-insensitive).
    /// Returns the first match found or null if not found.
    /// </summary>
    private TreeNode? FindNodeByPath(TreeNodeCollection nodes, string path)
    {
        foreach (TreeNode node in nodes)
        {
            if (node.Tag is string tag && string.Equals(tag, path, StringComparison.OrdinalIgnoreCase))
                return node;
            var found = FindNodeByPath(node.Nodes, path);
            if (found != null) return found;
        }
        return null;
    }

    private void LoadDirectoryNode(TreeNode parentNode, string path)
    {
        try
        {
            var dirInfo = new DirectoryInfo(path);
            
            // Add subdirectories
            foreach (var dir in dirInfo.GetDirectories().OrderBy(d => d.Name))
            {
                var subNode = new TreeNode(dir.Name)
                {
                    Tag = dir.FullName,
                    ImageIndex = 0,
                    SelectedImageIndex = 0
                };
                LoadDirectoryNode(subNode, dir.FullName);
                parentNode.Nodes.Add(subNode);
            }

            // Add files
            foreach (var file in dirInfo.GetFiles().OrderBy(f => f.Name))
            {
                var fileNode = new TreeNode(file.Name)
                {
                    Tag = file.FullName,
                    ImageIndex = 1,
                    SelectedImageIndex = 1
                };
                parentNode.Nodes.Add(fileNode);
            }
        }
        catch { }
    }

    private TreeNode? GetSelectedNode()
    {
        return treeView?.SelectedNode;
    }

    private string? GetSelectedPath()
    {
        var node = GetSelectedNode();
        return node?.Tag as string;
    }

    private bool IsDirectory(string? path)
    {
        return path != null && FileSystemUtils.IsDirectory(path);
    }

    private bool IsFile(string? path)
    {
        return path != null && FileSystemUtils.IsFile(path);
    }

    private void TreeView_NodeMouseClick(object? sender, TreeNodeMouseClickEventArgs e)
    {
        if (treeView != null)
        {
            treeView.SelectedNode = e.Node;
            var nodePath = e.Node?.Tag as string;
            if (!string.IsNullOrEmpty(nodePath))
            {
                if (IsDirectory(nodePath))
                    setCurrentWorkingPath(nodePath);
                else if (IsFile(nodePath))
                    setCurrentWorkingPath(Path.GetDirectoryName(nodePath) ?? rootPath);
            }
        }
    }

    private void TreeView_AfterSelect(object? sender, TreeViewEventArgs e)
    {
        var nodePath = e.Node?.Tag as string;
        if (string.IsNullOrEmpty(nodePath)) return;
        if (IsDirectory(nodePath))
            setCurrentWorkingPath(nodePath);
        else if (IsFile(nodePath))
            setCurrentWorkingPath(Path.GetDirectoryName(nodePath) ?? rootPath);
    }

    private void ShowFriendlyError(Exception ex, string defaultMessage)
    {
        // If FileSystemUtils produced a FileSystemException, show that friendly message.
        if (ex is FileSystemException fse)
        {
            MessageBox.Show(fse.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return;
        }

        // Log details for debugging, but present a concise friendly message to the user.
        // This ensures we capture technical details (full stack trace, inner exceptions) in the
        // debug output for developers, while the user sees only a simple, understandable message
        try { Debug.WriteLine(ex.ToString()); } catch { }
        MessageBox.Show(defaultMessage, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
    }

    private void OpenSelectedNode()
    {
        try
        {
            var path = GetSelectedPath();
            if (path == null) return;

            if (IsDirectory(path))
            {
                MessageBox.Show("This is a directory. Cannot open.");
                return;
            }

            if (IsFile(path))
            {
                if (txtContent != null && txtFileName != null)
                {
                    txtContent.Text = FileSystemUtils.ReadAllText(path);
                    txtFileName.Text = Path.GetFileName(path);
                    setCurrentWorkingPath(Path.GetDirectoryName(path) ?? rootPath);
                }
            }
        }
        catch (Exception ex)
        {
            ShowFriendlyError(ex, "Failed to open file.");
        }
    }

    private void CreateFile()
    {
        try
        {
            if (txtFileName == null || txtContent == null) return;
            var name = txtFileName.Text?.Trim();
            if (string.IsNullOrEmpty(name)) { MessageBox.Show("Enter a file name."); return; }
            FileSystemUtils.ValidateFileName(name);
            var path = Path.Combine(currentWorkingPath, name);
            if (FileSystemUtils.IsFile(path)) { MessageBox.Show("File already exists."); return; }
            FileSystemUtils.WriteAllText(path, txtContent.Text ?? string.Empty);
            RefreshTree();
        }
        catch (Exception ex)
        {
            ShowFriendlyError(ex, "Failed to create file.");
        }
    }

    private void CreateFolder()
    {
        try
        {
            var folderName = PromptForText("Create Folder", "Folder name:", "");
            if (string.IsNullOrEmpty(folderName)) return;
            var path = Path.Combine(currentWorkingPath, folderName);
            FileSystemUtils.ValidateFileName(folderName);
            if (FileSystemUtils.IsDirectory(path)) { MessageBox.Show("Folder already exists."); return; }
            FileSystemUtils.CreateDirectory(path);
            RefreshTree();
        }
        catch (Exception ex)
        {
            ShowFriendlyError(ex, "Failed to create folder.");
        }
    }

    private void CreateFolderInNode()
    {
        try
        {
            var selectedPath = GetSelectedPath();
            if (selectedPath == null) { MessageBox.Show("Select a location."); return; }

            string parentPath = IsDirectory(selectedPath) ? selectedPath : Path.GetDirectoryName(selectedPath) ?? rootPath;
            if (!FileSystemUtils.IsDirectory(parentPath)) { MessageBox.Show("Invalid parent directory."); return; }

            var folderName = PromptForText("Create Folder", "Folder name:", "");
            if (string.IsNullOrEmpty(folderName)) return;

            var path = Path.Combine(parentPath, folderName);
            FileSystemUtils.ValidateFileName(folderName);
            if (FileSystemUtils.IsDirectory(path)) { MessageBox.Show("Folder already exists."); return; }
            FileSystemUtils.CreateDirectory(path);
            RefreshTree();
        }
        catch (Exception ex)
        {
            ShowFriendlyError(ex, "Failed to create folder in selected location.");
        }
    }

    private void CreateFileInNode()
    {
        try
        {
            var selectedPath = GetSelectedPath();
            if (selectedPath == null) { MessageBox.Show("Select a location."); return; }

            string parentPath = IsDirectory(selectedPath) ? selectedPath : Path.GetDirectoryName(selectedPath) ?? rootPath;
            if (!FileSystemUtils.IsDirectory(parentPath)) { MessageBox.Show("Invalid parent directory."); return; }

            var fileName = PromptForText("Create File", "File name:", "");
            if (string.IsNullOrEmpty(fileName)) return;

            FileSystemUtils.ValidateFileName(fileName);
            var path = Path.Combine(parentPath, fileName);
            if (FileSystemUtils.IsFile(path)) { MessageBox.Show("File already exists."); return; }
            FileSystemUtils.WriteAllText(path, "");
            RefreshTree();
        }
        catch (Exception ex)
        {
            ShowFriendlyError(ex, "Failed to create file in selected location.");
        }
    }

    private void SaveFile()
    {
        try
        {
            if (txtFileName == null || txtContent == null) return;
            var name = txtFileName.Text?.Trim();
            if (string.IsNullOrEmpty(name)) { MessageBox.Show("Enter a file name."); return; }
            FileSystemUtils.ValidateFileName(name);
            var path = Path.Combine(currentWorkingPath, name);
            FileSystemUtils.WriteAllText(path, txtContent.Text ?? string.Empty);
            RefreshTree();
        }
        catch (Exception ex)
        {
            ShowFriendlyError(ex, "Failed to save file.");
        }
    }

    private void DeleteNode()
    {
        try
        {
            var path = GetSelectedPath();
            if (path == null) { MessageBox.Show("Select a file or folder to delete."); return; }

            if (path.Equals(rootPath, StringComparison.OrdinalIgnoreCase))
            {
                MessageBox.Show("Cannot delete the root folder.");
                return;
            }

            var itemName = Path.GetFileName(path);
            // Handle file vs directory deletion
            if (IsDirectory(path))
            {
                DeleteFolderWithOptions(path, itemName);
            }
            else if (IsFile(path))
            {
                var ok = MessageBox.Show("Delete '" + itemName + "'?", AppConstants.ConfirmTitle, MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (ok != DialogResult.Yes) return;
                FileSystemUtils.DeleteFile(path);
            }

            if (txtContent != null && txtFileName != null)
            {
                txtContent.Clear();
                txtFileName.Clear();
            }
            RefreshTree();
        }
        catch (Exception ex)
        {
            ShowFriendlyError(ex, "Failed to delete the selected item.");
        }
    }

    /// <summary>
    /// Prompt the user with delete options for a folder:
    /// 1. Yes = Delete empty only (fail if not empty)
    /// 2. No = Delete recursively (with confirmation)
    /// 3. Cancel = Abort
    /// </summary>
    private void DeleteFolderWithOptions(string folderPath, string folderName)
    {
        var result = MessageBox.Show(
            $"Delete folder '{folderName}'?\n\nYes = Delete (fail if not empty)\nNo = Delete recursively (with confirmation)\nCancel = Abort",
            AppConstants.ConfirmTitle,
            MessageBoxButtons.YesNoCancel,
            MessageBoxIcon.Question
        );

        if (result == DialogResult.Cancel)
            return; // User chose cancel, do nothing

        if (result == DialogResult.Yes)
        {
            // Delete empty only (non-recursive)
            FileSystemUtils.DeleteDirectory(folderPath, false);
        }
        else if (result == DialogResult.No)
        {
            // User chose recursive delete — ask for confirmation
            var confirm = MessageBox.Show(
                $"Permanently delete '{folderName}' and ALL its contents?\n\nThis cannot be undone.",
                AppConstants.ConfirmTitle,
                MessageBoxButtons.YesNo,
                MessageBoxIcon.Warning
            );
            if (confirm == DialogResult.Yes)
            {
                FileSystemUtils.DeleteDirectory(folderPath, true);
            }
            // else: user said no to the confirmation, do nothing
        }
    }

    private void RenameSelectedNode()
    {
        try
        {
            var path = GetSelectedPath();
            if (path == null) { MessageBox.Show("Select a file or folder to rename."); return; }

            var oldName = Path.GetFileName(path);
            var newName = PromptForText("Rename", "New name:", oldName);
            if (string.IsNullOrEmpty(newName) || newName == oldName) return;

            var parentPath = Path.GetDirectoryName(path) ?? rootPath;
            var newPath = Path.Combine(parentPath, newName);
            try
            {
                FileSystemUtils.Rename(path, newPath);
            }
            catch (Exception ex)
            {
                ShowFriendlyError(ex, "Failed to rename the selected item.");
                return;
            }
            RefreshTree();
        }
        catch (Exception ex)
        {
            ShowFriendlyError(ex, "Failed to rename the selected item.");
        }
    }

    private void RevealInExplorer()
    {
        try
        {
            var path = GetSelectedPath();
            if (path == null) { MessageBox.Show("Select a file or folder."); return; }
            FileSystemUtils.RevealInExplorer(path);
        }
        catch (Exception ex)
        {
            ShowFriendlyError(ex, "Failed to reveal the selected item in Explorer.");
        }
    }
    private string? PromptForText(string title, string label, string defaultValue)
    {
        using (var form = new Form() { Width = 520, Height = 150, Text = title, StartPosition = FormStartPosition.CenterParent })
        {
            var lbl = new Label() { Left = 8, Top = 8, Text = label, AutoSize = true };
            var txt = new TextBox() { Left = 8, Top = 30, Width = 480, Text = defaultValue ?? "" };
            var ok = new Button() { Text = "OK", Left = 320, Width = 75, Top = 64, DialogResult = DialogResult.OK };
            var cancel = new Button() { Text = "Cancel", Left = 405, Width = 75, Top = 64, DialogResult = DialogResult.Cancel };
            form.Controls.AddRange(new Control[] { lbl, txt, ok, cancel });
            form.AcceptButton = ok;
            form.CancelButton = cancel;
            if (form.ShowDialog(this) == DialogResult.OK)
                return txt.Text.Trim();
            return null;
        }
    }
}
