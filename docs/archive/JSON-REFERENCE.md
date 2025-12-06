# Blueprint Exporter JSON Reference

This document describes the JSON output format from the Blueprint Exporter plugin and provides query examples for analyzing exported blueprints.

## Table of Contents

- [JSON Schema](#json-schema)
- [Determinism](#determinism)
- [jq Query Examples](#jq-query-examples)
- [Common Use Cases](#common-use-cases)

---

## JSON Schema

### Top-Level Structure

```typescript
{
  "name": string,              // Blueprint asset name (e.g., "BP_Player")
  "path": string,              // Full asset path (e.g., "/Game/Characters/BP_Player.BP_Player")
  "class_type": string,        // Blueprint type (e.g., "BPTYPE_Normal", "BPTYPE_Interface")
  "parent_class": string,      // Parent class name (e.g., "Character", "Actor")
  "generated_class": string,   // Generated C++ class name
  "variables": Variable[],     // Blueprint variables
  "functions": Function[],     // Blueprint functions
  "graphs": Graph[],          // Event graphs (EventGraph, ConstructionScript, etc.)
  "components": Component[],   // Components (for Actor blueprints)
  "dependencies": string[]     // Asset dependencies (paths to referenced assets)
}
```

### Variable Object

```typescript
{
  "name": string,              // Variable name
  "type": string,              // Type (e.g., "float", "int", "Actor Object Reference")
  "category": string,          // Category for organization in editor
  "is_exposed": boolean,       // True if exposed on spawn
  "default_value"?: string     // Default value if set (optional)
}
```

**Example:**
```json
{
  "name": "MaxHealth",
  "type": "float",
  "category": "Combat",
  "is_exposed": true,
  "default_value": "100.0"
}
```

### Function Object

```typescript
{
  "name": string,              // Function name
  "parameters": Parameter[],   // Input and output parameters
  "return_type"?: string,      // Return type if function returns a value (optional)
  "graph": Graph              // Function's execution graph
}
```

### Parameter Object

```typescript
{
  "name": string,              // Parameter name
  "type": string,              // Parameter type
  "direction": "input" | "output"  // Parameter direction
}
```

**Example:**
```json
{
  "name": "TakeDamage",
  "parameters": [
    {
      "name": "DamageAmount",
      "type": "float",
      "direction": "input"
    },
    {
      "name": "Success",
      "type": "boolean",
      "direction": "output"
    }
  ],
  "return_type": "boolean",
  "graph": { /* ... */ }
}
```

### Graph Object

```typescript
{
  "name": string,              // Graph name (e.g., "EventGraph", "ConstructionScript")
  "nodes": Node[]             // Array of nodes in this graph
}
```

### Node Object

```typescript
{
  "id": string,                // Unique node identifier (e.g., "K2Node_Event_123")
  "type": string,              // Node type (e.g., "Event", "FunctionCall", "Branch", "VariableGet")
  "title": string,             // Display title shown in editor (e.g., "Event BeginPlay")
  "category": string,          // Node category
  "position": {
    "x": number,               // X position in graph editor
    "y": number                // Y position in graph editor
  },
  "pins": Pin[],              // Array of input/output pins
  "connections": string[]      // Array of connected node IDs (high-level view)
}
```

**Common node types:**
- `Event` - Event nodes (BeginPlay, Tick, etc.)
- `FunctionCall` - Function call nodes
- `Branch` - Conditional branch
- `VariableGet` - Get variable value
- `VariableSet` - Set variable value
- `Sequence` - Sequence node (multiple exec outputs)
- `ForEachLoop` - For-each loop
- `Timeline` - Timeline node

### Pin Object

```typescript
{
  "name": string,              // Internal pin name
  "display_name": string,      // User-friendly display name
  "direction": "input" | "output",
  "type": string,              // Pin type (see Pin Types below)
  "default_value"?: string,    // Default value if set (optional)
  "connected_to"?: Connection[] // Array of pin connections (optional)
}
```

**Common pin types:**
- `exec` - Execution flow pin (white)
- `boolean` - Boolean (red)
- `byte` - Byte (blue-green)
- `int` - Integer (cyan)
- `float` - Float (green)
- `string` - String (magenta)
- `name` - Name (purple)
- `text` - Text (pink)
- `vector` - Vector (yellow)
- `rotator` - Rotator (light blue)
- `transform` - Transform (orange)
- `Object` - Object reference (blue)
- `Actor` - Actor reference (blue)

### Connection Object

```typescript
{
  "node_id": string,           // ID of connected node
  "node_title": string,        // Title of connected node
  "pin_name": string,          // Name of connected pin
  "pin_display_name": string   // Display name of connected pin
}
```

**Example:**
```json
{
  "node_id": "K2Node_CallFunction_456",
  "node_title": "Print String",
  "pin_name": "execute",
  "pin_display_name": "Execute"
}
```

### Component Object

```typescript
{
  "name": string,              // Component name
  "class": string,             // Component class (e.g., "StaticMeshComponent")
  "parent"?: string            // Parent component name if nested (optional)
}
```

**Example:**
```json
{
  "name": "WeaponMesh",
  "class": "StaticMeshComponent",
  "parent": "RootComponent"
}
```

---

## Determinism

The exporter produces **fully deterministic output**:

- ✅ Nodes are exported in execution-flow order (starting from entry points, following exec pins)
- ✅ Dependencies are sorted alphabetically
- ✅ Components are sorted alphabetically
- ✅ No timestamps, GUIDs, or random data

This ensures that:
- **Git diffs show only logical changes** - Moving nodes visually only changes the `position` field
- **Identical blueprints produce identical JSON** - Deterministic output enables reliable diffing and version control
- **Related nodes cluster together** - Execution-flow ordering makes the JSON readable and semantically meaningful

---

## jq Query Examples

All examples use the `-c` flag for compact output suitable for programmatic processing.

### Basic Queries

**Get blueprint name:**
```bash
jq -c '.name' BP_Player.json
```

**Get blueprint type:**
```bash
jq -c '.class_type' BP_Player.json
```

**Get parent class:**
```bash
jq -c '.parent_class' BP_Player.json
```

### Variable Queries

**List all variable names:**
```bash
jq -c '[.variables[].name]' BP_Player.json
```

**Get all variables:**
```bash
jq -c '.variables[]' BP_Player.json
```

**Get variables of specific type:**
```bash
jq -c '[.variables[] | select(.type == "float")]' BP_Player.json
```

**Get exposed variables only:**
```bash
jq -c '[.variables[] | select(.is_exposed == true)]' BP_Player.json
```

**Find variable by name:**
```bash
jq -c '.variables[] | select(.name == "MaxHealth")' BP_Player.json
```

### Function Queries

**List all function names:**
```bash
jq -c '[.functions[].name]' BP_Player.json
```

**Get function details:**
```bash
jq -c '.functions[] | select(.name == "TakeDamage")' BP_Player.json
```

**List all function parameters:**
```bash
jq -c '.functions[] | {name: .name, params: [.parameters[].name]}' BP_Player.json
```

### Node Queries

**List all node IDs:**
```bash
jq -c '[.graphs[].nodes[].id]' BP_Player.json
```

**List all node titles:**
```bash
jq -c '[.graphs[].nodes[].title]' BP_Player.json
```

**Get node by ID:**
```bash
jq -c '.graphs[].nodes[] | select(.id == "K2Node_Event_123")' BP_Player.json
```

**Get node by title:**
```bash
jq -c '.graphs[].nodes[] | select(.title == "Event BeginPlay")' BP_Player.json
```

**Find nodes containing text in title:**
```bash
jq -c '[.graphs[].nodes[] | select(.title | contains("Movement"))]' BP_Player.json
```

**Get all Event nodes:**
```bash
jq -c '[.graphs[].nodes[] | select(.type == "Event")]' BP_Player.json
```

**Get all FunctionCall nodes:**
```bash
jq -c '[.graphs[].nodes[] | select(.type == "FunctionCall")]' BP_Player.json
```

**Get all Branch nodes:**
```bash
jq -c '[.graphs[].nodes[] | select(.type == "Branch")]' BP_Player.json
```

**Count nodes by type:**
```bash
jq -c '[.graphs[].nodes[].type] | group_by(.) | map({type: .[0], count: length})' BP_Player.json
```

### Pin and Connection Queries

**Get all pins for a node:**
```bash
jq -c '.graphs[].nodes[] | select(.id == "K2Node_Event_123") | .pins[]' BP_Player.json
```

**Get input pins only:**
```bash
jq -c '.graphs[].nodes[] | select(.id == "K2Node_Event_123") | [.pins[] | select(.direction == "input")]' BP_Player.json
```

**Get output pins only:**
```bash
jq -c '.graphs[].nodes[] | select(.id == "K2Node_Event_123") | [.pins[] | select(.direction == "output")]' BP_Player.json
```

**Get exec pins:**
```bash
jq -c '.graphs[].nodes[] | select(.id == "K2Node_Event_123") | [.pins[] | select(.type == "exec")]' BP_Player.json
```

**Find nodes connected TO a node's inputs:**
```bash
jq -c '.graphs[].nodes[] | select(.id == "K2Node_Event_123") | [.pins[] | select(.direction == "input") | .connected_to[]?.node_id] | unique' BP_Player.json
```

**Find nodes connected FROM a node's outputs:**
```bash
jq -c '.graphs[].nodes[] | select(.id == "K2Node_Event_123") | [.pins[] | select(.direction == "output") | .connected_to[]?.node_id] | unique' BP_Player.json
```

**Get all nodes connected via exec flow:**
```bash
jq -c '.graphs[].nodes[] | select(.id == "K2Node_Event_123") | [.pins[] | select(.type == "exec" and .direction == "output") | .connected_to[]?]' BP_Player.json
```

**Build connection map:**
```bash
jq -c '[.graphs[].nodes[] | {id: .id, title: .title, connects_to: .connections}]' BP_Player.json
```

### Component Queries

**List all component names:**
```bash
jq -c '[.components[].name]' BP_Player.json
```

**Get component by name:**
```bash
jq -c '.components[] | select(.name == "WeaponMesh")' BP_Player.json
```

**Get components by class:**
```bash
jq -c '[.components[] | select(.class == "StaticMeshComponent")]' BP_Player.json
```

### Dependency Queries

**List all dependencies:**
```bash
jq -c '.dependencies[]' BP_Player.json
```

**Count dependencies:**
```bash
jq -c '.dependencies | length' BP_Player.json
```

**Find specific dependency:**
```bash
jq -c '.dependencies[] | select(contains("GameplayAbilities"))' BP_Player.json
```

### Graph Queries

**List all graph names:**
```bash
jq -c '[.graphs[].name]' BP_Player.json
```

**Get specific graph:**
```bash
jq -c '.graphs[] | select(.name == "EventGraph")' BP_Player.json
```

**Count nodes in each graph:**
```bash
jq -c '[.graphs[] | {name: .name, node_count: (.nodes | length)}]' BP_Player.json
```

---

## Common Use Cases

### 1. Find Entry Point and Trace Execution

```bash
# Find Event BeginPlay
jq -c '.graphs[].nodes[] | select(.title | contains("BeginPlay"))' BP_Player.json

# Get its exec output connections
jq -c '.graphs[].nodes[] | select(.title | contains("BeginPlay")) | .pins[] | select(.type == "exec" and .direction == "output") | .connected_to[]?' BP_Player.json
```

### 2. Find All Variable References

```bash
# Find all nodes that get or set a variable
jq -c '[.graphs[].nodes[] | select(.type == "VariableGet" or .type == "VariableSet") | select(.title | contains("MaxHealth"))]' BP_Player.json
```

### 3. Find All Print/Debug Nodes

```bash
jq -c '[.graphs[].nodes[] | select(.title | contains("Print"))]' BP_Player.json
```

### 4. Analyze Function Complexity

```bash
# Count nodes in each function
jq -c '[.functions[] | {name: .name, complexity: (.graph.nodes | length)}]' BP_Player.json
```

### 5. Find Disconnected Nodes

```bash
# Nodes with no connections
jq -c '[.graphs[].nodes[] | select(.connections | length == 0)]' BP_Player.json
```

### 6. Extract Just Node Names for Overview

```bash
jq -c '{blueprint: .name, events: [.graphs[].nodes[] | select(.type == "Event") | .title]}' BP_Player.json
```

### 7. Find All Casts

```bash
jq -c '[.graphs[].nodes[] | select(.title | contains("Cast to"))]' BP_Player.json
```

### 8. Compare Two Blueprint Versions

```bash
# Export node IDs from both versions
jq -c '[.graphs[].nodes[].id]' BP_Player_v1.json > v1_nodes.json
jq -c '[.graphs[].nodes[].id]' BP_Player_v2.json > v2_nodes.json

# Find added/removed nodes using diff or jq
jq -c --slurpfile v1 v1_nodes.json --slurpfile v2 v2_nodes.json -n '$v2[0] - $v1[0]'
```

### 9. Generate Dependency Graph

```bash
# Create adjacency list
jq -c '[.graphs[].nodes[] | {from: .id, to: [.pins[] | select(.direction == "output") | .connected_to[]?.node_id] | unique}]' BP_Player.json
```

### 10. Find Performance Bottlenecks

```bash
# Find Tick events (can be expensive)
jq -c '[.graphs[].nodes[] | select(.title | contains("Tick"))]' BP_Player.json

# Find loops
jq -c '[.graphs[].nodes[] | select(.type | contains("Loop"))]' BP_Player.json
```

---

## Tips for Claude Code Integration

When using these queries with Claude Code:

1. **Pipe results to further processing:**
   ```bash
   jq -c '.graphs[].nodes[] | select(.type == "Event")' BP_Player.json | jq -c '.title'
   ```

2. **Combine with grep for text searches (omit `-c`):**
   ```bash
   jq '.graphs[].nodes[]' BP_Player.json | grep -i "movement"
   ```

3. **Count results:**
   ```bash
   jq -c '[.graphs[].nodes[] | select(.type == "Branch")] | length' BP_Player.json
   ```

4. **Pretty print for human reading (omit `-c`):**
   ```bash
   jq '.graphs[].nodes[] | select(.id == "K2Node_Event_123")' BP_Player.json
   ```

5. **Export subsets for focused analysis:**
   ```bash
   jq -c '{events: [.graphs[].nodes[] | select(.type == "Event")], variables: .variables}' BP_Player.json > summary.json
   ```
