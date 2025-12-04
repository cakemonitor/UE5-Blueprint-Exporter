"""
Export Blueprints - Simple script to export all blueprints

Usage in UE5 Editor Python console:
    py "Content/Python/export_blueprints.py"

Or if you have the plugin loaded, just type:
    export_blueprints()
"""

import sys
import os

# Add current directory to path so we can import blueprint_watcher
current_dir = os.path.dirname(os.path.abspath(__file__))
if current_dir not in sys.path:
    sys.path.append(current_dir)

# Import and run the main export function
import blueprint_watcher
blueprint_watcher.main()
