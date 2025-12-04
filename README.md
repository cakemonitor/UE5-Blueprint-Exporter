# UE5 Blueprint Exporter

Export Unreal Engine 5 blueprints to JSON and Markdown files for documentation and analysis.

## Installation

1. Copy `Plugins/BlueprintExporter/` to your UE5 project
2. Open your project in Unreal Editor
3. Click "Yes" when prompted to rebuild the plugin modules
4. Plugin will appear under Edit → Plugins once loaded

## Usage

### Export via Menu
**Tools → Export Blueprint Documentation**

Uses settings from Edit → Project Settings → Plugins → Blueprint Exporter

### Export via Python Console
Open Output Log → Python Console tab, then run:
```python
export_blueprints()
```

### Output Location
Exports to `YourProject/Exported/Blueprints/` by default, preserving your project's folder structure.

Each blueprint generates:
- `BlueprintName.json` - Complete graph data, nodes, pins, connections
- `BlueprintName.md` - Human-readable markdown documentation

## Configuration

Edit → Project Settings → Plugins → Blueprint Exporter

- **Output Directory**: Where to export files (default: `Exported/Blueprints`)
- **Generate Markdown Files**: Create .md files alongside JSON (default: enabled)
- **Pretty Print JSON**: Format JSON with new lines and indentation (default: enabled)

## Requirements

- Unreal Engine 5.0 or later
- C++ build tools (Visual Studio on Windows, Xcode on macOS)
- Python support enabled in UE5 (included by default)
