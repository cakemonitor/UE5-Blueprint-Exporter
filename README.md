# UE5 Blueprint Documentation Exporter

> **Export Unreal Engine 5 blueprints to JSON and Markdown documentation**

A lightweight C++ plugin that extracts complete blueprint data for documentation, analysis, version control, and AI-powered assistance.

[![UE5](https://img.shields.io/badge/UE5-5.0+-0E1128?logo=unrealengine)](https://www.unrealengine.com/)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Mac%20%7C%20Linux-lightgrey)](#)

---

## ✨ What Does This Do?

**Problem:** UE5 blueprints are visual and locked inside `.uasset` files. You can't:
- Search across all blueprints easily
- Generate documentation automatically
- Track blueprint changes in version control
- Use external analysis tools

**Solution:** This plugin extracts complete blueprint data to readable formats:

```
Your Blueprints → This Plugin → JSON + Markdown Files
```

**Use cases:**
- 🤖 **AI Assistants** - Ask Claude, ChatGPT questions about your blueprint logic
- 📚 **Documentation Sites** - Generate static documentation websites
- 🔍 **Search & Analysis** - Build custom search and analysis tools
- 📊 **Version Control** - Meaningful blueprint diffs in Git
- 🔧 **External Tools** - Integrate with any tool that reads JSON

---

## 🚀 Quick Start

### Prerequisites
- **Unreal Engine 5.0 or higher**
- **C++ compiler** (Visual Studio on Windows, Xcode on Mac, comes with UE5 on Linux)

### Installation (3 Steps)

#### 1. Copy the Plugin
```bash
git clone https://github.com/YOUR_USERNAME/UE5-Blueprint-Exporter.git
cp -r UE5-Blueprint-Exporter/Plugins/BlueprintExporter /path/to/YourProject/Plugins/
```

**That's it!** The plugin is self-contained - Python scripts are included in the plugin folder.

#### 2. Open Your Project
Open `YourProject.uproject` in UE5 Editor

You'll see: *"Missing modules. Would you like to rebuild them now?"*

Click **Yes** and wait for compilation (1-2 minutes)

#### 3. Export Your Blueprints

**GUI Method (Easiest):**
- **Tools → Export Blueprint Documentation**
- Check Output Log for results

**Python Console Method:**
- Open Output Log: **Window → Developer Tools → Output Log**
- Type in Python console: `export_blueprints()`
- Press Enter
- No imports or file paths needed - the plugin registers everything automatically!

**Done!** Your blueprints are now in `YourProject/Exported/Blueprints/`

---

## 📂 Output Structure

```
YourProject/Exported/Blueprints/
├── index.md                        # Overview with all blueprints
│
├── Characters/
│   ├── BP_Player.json             # Complete blueprint data
│   ├── BP_Player.md               # Human-readable summary
│   ├── BP_Enemy.json
│   └── BP_Enemy.md
│
└── Weapons/
    ├── BP_Rifle.json
    └── BP_Rifle.md
```

### Sample Output

**Markdown (`BP_Player.md`):**
```markdown
# BP_Player

**Type:** Blueprint
**Parent Class:** Character

## Variables
| Name | Type | Default |
|------|------|---------|
| Health | float | 100.0 |
| MaxSpeed | float | 600.0 |

## Functions
- `TakeDamage(float Amount)`
- `Heal(float Amount)`

## Components
- CameraComponent
- SpringArmComponent
- SkeletalMeshComponent
```

**JSON (`BP_Player.json`):**
Complete graph data including all nodes, pins, connections, and dependencies.

---

## 📋 What Gets Exported

| Data | Description |
|------|-------------|
| **Graphs** | Event graphs, functions, macros, delegates |
| **Nodes** | Type, title, category, position |
| **Pins** | Name, direction, type, default values |
| **Connections** | Complete execution flow |
| **Variables** | Name, type, category, defaults |
| **Functions** | Parameters, return types, implementation |
| **Components** | Hierarchy and class info |
| **Dependencies** | All referenced assets and blueprints |

---

## 💬 Using with AI Assistants

### With Claude Code

```bash
cd /path/to/your/ue5/project
claude-code
```

Then ask:
- *"How does BP_Player handle movement input?"*
- *"What blueprints depend on BP_GameMode?"*
- *"Show me all weapons and their damage values"*

### With ChatGPT / Other AI

Upload the exported Markdown files and ask questions about your blueprint logic.

---

## ⚙️ Configuration

Edit `Plugins/BlueprintExporter/Content/Python/blueprint_watcher.py`:

```python
# Output directory (relative to project root)
OUTPUT_DIR = "Exported/Blueprints"

# Generate markdown files
GENERATE_MARKDOWN = True

# Include detailed node information
INCLUDE_GRAPH_NODES = True
```

---

## 🛠️ System Requirements

| Component | Requirement |
|-----------|-------------|
| **Unreal Engine** | 5.0 or higher |
| **C++ Compiler** | Visual Studio 2019+ (Win), Xcode (Mac), Included with UE5 (Linux) |
| **Disk Space** | ~10MB for plugin, varies for exports |

---

## 🐛 Troubleshooting

### Plugin Doesn't Appear in Menu

**Check:** Edit → Plugins → Search "Blueprint" → Ensure "Blueprint Documentation Exporter" is enabled

### Compilation Fails

**Solution:** Make sure you have the correct compiler installed:
- **Windows:** Visual Studio 2019 or 2022 with C++ tools
- **Mac:** Xcode with Command Line Tools
- **Linux:** UE5 uses bundled clang

### Python Command Not Found

**Solution:**
1. Ensure plugin is enabled (Edit → Plugins)
2. Restart the editor
3. Check Output Log for "Python command registered: export_blueprints()" message
4. If still not working, the plugin Python path may not be registered - check that Python Editor Script Plugin is enabled

### No Output Files

**Check Output Log:** Window → Developer Tools → Output Log

Look for error messages starting with `LogBlueprintExporter:`

---

## 📖 Documentation

- **[SETUP_INSTRUCTIONS.md](SETUP_INSTRUCTIONS.md)** - Detailed setup and troubleshooting
- **[BLUEPRINT_EXPORTER_README.md](BLUEPRINT_EXPORTER_README.md)** - Technical architecture details

---

## 🤝 Contributing

Contributions welcome! This is an open-source tool for the UE5 community.

**To contribute:**
1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Commit your changes: `git commit -m 'Add amazing feature'`
4. Push to branch: `git push origin feature/amazing-feature`
5. Open a Pull Request

---

## 📜 License

MIT License - See [LICENSE](LICENSE) file for details.

---

## 🌟 Star This Repository!

If this tool helps you, please **⭐ star the repository** to help others discover it!

---

**Made with ❤️ for the UE5 community**

[Report Bug](https://github.com/YOUR_USERNAME/UE5-Blueprint-Exporter/issues) · [Request Feature](https://github.com/YOUR_USERNAME/UE5-Blueprint-Exporter/issues)
