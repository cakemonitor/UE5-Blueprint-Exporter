# Blueprint JSON Schema

## Structure

```
{
  name: string              // Blueprint name (e.g., "BP_Player")
  path: string              // Asset path
  class_type: string        // Blueprint type
  parent_class: string      // Parent class
  generated_class: string   // Generated C++ class
  variables: Variable[]     // Blueprint variables
  functions: Function[]     // Blueprint functions
  graphs: Graph[]           // Event graphs
  components: Component[]   // Actor components
  dependencies: string[]    // Asset dependencies
}
```

### Variable
```
{ name: string, type: string, category: string, is_exposed: boolean, default_value?: string }
```

### Function
```
{ name: string, parameters: Parameter[], graph: Graph }
```

### Graph
```
{ name: string, nodes: Node[] }
```

### Node
```
{
  id: string                // Unique ID (e.g., "K2Node_Event_123")
  type: string              // "Event", "FunctionCall", "Branch", "VariableGet", etc.
  title: string             // Display name
  pins: Pin[]               // Input/output pins (excludes delegate pins)
}
```

### Pin
```
{
  name: string
  direction: "input" | "output"
  type: string              // "exec", "float", "int", "boolean", "Object", etc.
  default_value?: string
  to?: Connection[]         // Connected pins
}
```

### Connection
```
{
  node: string              // Target node ID
  pin: string               // Target pin name
}
```

### Component
```
{ name: string, class: string }
```

## Determinism

- Nodes exported in **execution-flow order** (entry points → exec flow → remaining)
- Components and dependencies **sorted alphabetically**
- Identical blueprints = identical JSON

## Common jq Queries For Programmatic Processing

```bash
# Get all node titles
jq -c '[.graphs[].nodes[].title]' BP_Player.json

# Find specific node by title
jq -c '.graphs[].nodes[] | select(.title == "Event BeginPlay")' BP_Player.json

# Get all variables of type float
jq -c '[.variables[] | select(.type == "float")]' BP_Player.json

# List all Event nodes
jq -c '[.graphs[].nodes[] | select(.type == "Event")]' BP_Player.json

# Find nodes connected FROM a node's exec outputs
jq -c '.graphs[].nodes[] | select(.id == "K2Node_Event_123") | [.pins[] | select(.type == "exec" and .direction == "output") | .to[]?.node]' BP_Player.json

# Count nodes by type
jq -c '[.graphs[].nodes[].type] | group_by(.) | map({type: .[0], count: length})' BP_Player.json

# Find all variable references
jq -c '[.graphs[].nodes[] | select(.type == "VariableGet" or .type == "VariableSet")]' BP_Player.json

# Search node titles (pipe to grep without -c)
jq '.graphs[].nodes[]' BP_Player.json | grep -i "movement"
```
