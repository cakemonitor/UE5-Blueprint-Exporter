// BlueprintExporter.cpp

#include "BlueprintExporter.h"
#include "Modules/ModuleManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ToolMenus.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"

// Python plugin headers (only if available)
#if defined(WITH_PYTHON) && WITH_PYTHON
#include "IPythonScriptPlugin.h"
#endif

// Define custom log category
DEFINE_LOG_CATEGORY(LogBlueprintExporter);

// ============================================================================
// Main Export Functions
// ============================================================================

FString UBlueprintExporterLibrary::ExtractBlueprintData(UBlueprint* Blueprint, bool bPrettyPrint)
{
	if (!Blueprint)
	{
		UE_LOG(LogBlueprintExporter, Error, TEXT("ExtractBlueprintData: Invalid blueprint"));
		return TEXT("{}");
	}

	TSharedPtr<FJsonObject> JsonObject = SerializeBlueprint(Blueprint);
	if (!JsonObject.IsValid())
	{
		UE_LOG(LogBlueprintExporter, Error, TEXT("ExtractBlueprintData: Failed to serialize blueprint"));
		return TEXT("{}");
	}

	// Convert to string
	FString OutputString;
	bool bSuccess = false;

	if (bPrettyPrint)
	{
		TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&OutputString);
		bSuccess = FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
	}
	else
	{
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&OutputString);
		bSuccess = FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
	}

	if (!bSuccess)
	{
		UE_LOG(LogBlueprintExporter, Error, TEXT("ExtractBlueprintData: Failed to serialize JSON to string"));
		return TEXT("{}");
	}

	return OutputString;
}

bool UBlueprintExporterLibrary::ExportBlueprintToFile(UBlueprint* Blueprint, const FString& FilePath, bool bPrettyPrint)
{
	if (!Blueprint)
	{
		UE_LOG(LogBlueprintExporter, Error, TEXT("ExportBlueprintToFile: Invalid blueprint"));
		return false;
	}

	// Validate file path
	if (FilePath.IsEmpty())
	{
		UE_LOG(LogBlueprintExporter, Error, TEXT("ExportBlueprintToFile: Empty file path provided"));
		return false;
	}

	// Ensure the directory exists
	FString Directory = FPaths::GetPath(FilePath);
	if (!Directory.IsEmpty())
	{
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (!PlatformFile.DirectoryExists(*Directory))
		{
			if (!PlatformFile.CreateDirectoryTree(*Directory))
			{
				UE_LOG(LogBlueprintExporter, Error, TEXT("ExportBlueprintToFile: Failed to create directory: %s"), *Directory);
				return false;
			}
		}
	}

	FString JsonString = ExtractBlueprintData(Blueprint, bPrettyPrint);

	// Save to file
	if (FFileHelper::SaveStringToFile(JsonString, *FilePath))
	{
		UE_LOG(LogBlueprintExporter, Log, TEXT("Exported blueprint to: %s"), *FilePath);
		return true;
	}

	UE_LOG(LogBlueprintExporter, Error, TEXT("Failed to save file: %s"), *FilePath);
	return false;
}

int32 UBlueprintExporterLibrary::ExportAllBlueprints(const FString& OutputDirectory, bool bPrettyPrint, bool bGenerateMarkdown)
{
	if (OutputDirectory.IsEmpty())
	{
		UE_LOG(LogBlueprintExporter, Error, TEXT("ExportAllBlueprints: Empty output directory provided"));
		return 0;
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*OutputDirectory))
	{
		if (!PlatformFile.CreateDirectoryTree(*OutputDirectory))
		{
			UE_LOG(LogBlueprintExporter, Error, TEXT("ExportAllBlueprints: Failed to create output directory: %s"), *OutputDirectory);
			return 0;
		}
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), AssetDataList);

	FScopedSlowTask Progress(AssetDataList.Num(), FText::FromString("Exporting Blueprints"));
	Progress.MakeDialog();

	int32 ExportedCount = 0;
	int32 FailedCount = 0;

	for (const FAssetData& AssetData : AssetDataList)
	{
		Progress.EnterProgressFrame(1.0f, FText::FromString(FString::Printf(TEXT("Exporting %s"), *AssetData.AssetName.ToString())));

		UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
		if (!Blueprint)
		{
			FailedCount++;
			continue;
		}

		// Get asset path and convert to output path with folder structure
		// e.g., /Game/Characters/BP_Player -> OutputDirectory/Characters/BP_Player.json
		FString AssetPath = AssetData.PackageName.ToString();
		FString RelativePath = AssetPath;

		// Remove /Game/ prefix
		if (RelativePath.StartsWith(TEXT("/Game/")))
		{
			RelativePath = RelativePath.RightChop(6); // Remove "/Game/"
		}

		// Determine output path (with or without subdirectories)
		FString Directory;
		FString FileName = Blueprint->GetName();
		FString TargetDirectory = OutputDirectory;

		if (RelativePath.Split(TEXT("/"), &Directory, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
		{
			// Has subdirectories
			TargetDirectory = FPaths::Combine(OutputDirectory, Directory);
			if (!PlatformFile.DirectoryExists(*TargetDirectory))
			{
				PlatformFile.CreateDirectoryTree(*TargetDirectory);
			}
		}

		// Export JSON
		FString JsonPath = FPaths::Combine(TargetDirectory, FileName + TEXT(".json"));
		if (ExportBlueprintToFile(Blueprint, JsonPath, bPrettyPrint))
		{
			ExportedCount++;
		}
		else
		{
			FailedCount++;
			continue;
		}

		// Export Markdown if requested
		if (bGenerateMarkdown)
		{
			FString MarkdownPath = FPaths::Combine(TargetDirectory, FileName + TEXT(".md"));
			if (!ExportBlueprintToMarkdown(Blueprint, MarkdownPath))
			{
				UE_LOG(LogBlueprintExporter, Warning, TEXT("Failed to export markdown for: %s"), *FileName);
			}
		}
	}

	UE_LOG(LogBlueprintExporter, Log, TEXT("Exported %d blueprints to %s (%d failed)"), ExportedCount, *OutputDirectory, FailedCount);
	return ExportedCount;
}

int32 UBlueprintExporterLibrary::ExportAllBlueprintsWithConfig(const FBlueprintExportConfig& Config)
{
	FString ProjectDir = FPaths::ProjectDir();
	FString OutputDir = FPaths::Combine(ProjectDir, Config.OutputDirectory);
	return ExportAllBlueprints(OutputDir, Config.bPrettyPrintJson, Config.bGenerateMarkdown);
}

bool UBlueprintExporterLibrary::ExportBlueprintToMarkdown(UBlueprint* Blueprint, const FString& FilePath)
{
	if (!Blueprint)
	{
		UE_LOG(LogBlueprintExporter, Error, TEXT("ExportBlueprintToMarkdown: Invalid blueprint"));
		return false;
	}

	TSharedPtr<FJsonObject> JsonObject = SerializeBlueprint(Blueprint);
	FString MarkdownContent = GenerateMarkdownFromJson(JsonObject, Blueprint);

	// Save to file
	if (FFileHelper::SaveStringToFile(MarkdownContent, *FilePath))
	{
		UE_LOG(LogBlueprintExporter, Log, TEXT("Exported markdown to: %s"), *FilePath);
		return true;
	}

	UE_LOG(LogBlueprintExporter, Error, TEXT("Failed to save markdown file: %s"), *FilePath);
	return false;
}

FString UBlueprintExporterLibrary::GenerateMarkdownFromJson(TSharedPtr<FJsonObject> JsonObject, UBlueprint* Blueprint)
{
	FString Markdown;

	// Header
	FString Name = JsonObject->GetStringField(TEXT("name"));
	FString ClassType = JsonObject->GetStringField(TEXT("class_type"));
	FString Path = JsonObject->GetStringField(TEXT("path"));
	FString ParentClass = JsonObject->HasField(TEXT("parent_class")) ? JsonObject->GetStringField(TEXT("parent_class")) : TEXT("None");

	Markdown += FString::Printf(TEXT("# %s\n\n"), *Name);
	Markdown += FString::Printf(TEXT("**Type:** %s  \n"), *ClassType);
	Markdown += FString::Printf(TEXT("**Path:** `%s`  \n"), *Path);
	Markdown += FString::Printf(TEXT("**Parent Class:** %s  \n\n"), *ParentClass);

	// Components
	const TArray<TSharedPtr<FJsonValue>>* ComponentsArray;
	if (JsonObject->TryGetArrayField(TEXT("components"), ComponentsArray) && ComponentsArray->Num() > 0)
	{
		Markdown += TEXT("## Components\n\n");
		for (const TSharedPtr<FJsonValue>& ComponentValue : *ComponentsArray)
		{
			TSharedPtr<FJsonObject> ComponentObj = ComponentValue->AsObject();
			FString CompName = ComponentObj->GetStringField(TEXT("name"));
			FString CompClass = ComponentObj->GetStringField(TEXT("class"));
			Markdown += FString::Printf(TEXT("- **%s** (%s)\n"), *CompName, *CompClass);
		}
		Markdown += TEXT("\n");
	}

	// Variables
	const TArray<TSharedPtr<FJsonValue>>* VariablesArray;
	if (JsonObject->TryGetArrayField(TEXT("variables"), VariablesArray) && VariablesArray->Num() > 0)
	{
		Markdown += TEXT("## Variables\n\n");
		Markdown += TEXT("| Name | Type | Category | Exposed |\n");
		Markdown += TEXT("|------|------|----------|---------|\n");
		for (const TSharedPtr<FJsonValue>& VarValue : *VariablesArray)
		{
			TSharedPtr<FJsonObject> VarObj = VarValue->AsObject();
			FString VarName = VarObj->GetStringField(TEXT("name"));
			FString VarType = VarObj->GetStringField(TEXT("type"));
			FString VarCategory = VarObj->GetStringField(TEXT("category"));
			bool bIsExposed = VarObj->GetBoolField(TEXT("is_exposed"));
			Markdown += FString::Printf(TEXT("| %s | %s | %s | %s |\n"),
				*VarName, *VarType, *VarCategory, bIsExposed ? TEXT("Yes") : TEXT("No"));
		}
		Markdown += TEXT("\n");
	}

	// Functions
	const TArray<TSharedPtr<FJsonValue>>* FunctionsArray;
	if (JsonObject->TryGetArrayField(TEXT("functions"), FunctionsArray) && FunctionsArray->Num() > 0)
	{
		Markdown += TEXT("## Functions\n\n");
		for (const TSharedPtr<FJsonValue>& FuncValue : *FunctionsArray)
		{
			TSharedPtr<FJsonObject> FuncObj = FuncValue->AsObject();
			FString FuncName = FuncObj->GetStringField(TEXT("name"));

			// Build parameters string
			FString ParamsStr;
			const TArray<TSharedPtr<FJsonValue>>* ParamsArray;
			if (FuncObj->TryGetArrayField(TEXT("parameters"), ParamsArray))
			{
				for (int32 i = 0; i < ParamsArray->Num(); i++)
				{
					TSharedPtr<FJsonObject> ParamObj = (*ParamsArray)[i]->AsObject();
					FString ParamName = ParamObj->GetStringField(TEXT("name"));
					FString ParamType = ParamObj->GetStringField(TEXT("type"));
					ParamsStr += FString::Printf(TEXT("%s: %s"), *ParamName, *ParamType);
					if (i < ParamsArray->Num() - 1)
					{
						ParamsStr += TEXT(", ");
					}
				}
			}

			Markdown += FString::Printf(TEXT("### %s(%s)\n\n"), *FuncName, *ParamsStr);

			// Include basic graph info
			const TSharedPtr<FJsonObject>* GraphObjPtr;
			if (FuncObj->TryGetObjectField(TEXT("graph"), GraphObjPtr))
			{
				const TArray<TSharedPtr<FJsonValue>>* NodesArray;
				if ((*GraphObjPtr)->TryGetArrayField(TEXT("nodes"), NodesArray))
				{
					Markdown += FString::Printf(TEXT("**Nodes:** %d\n\n"), NodesArray->Num());
				}
			}
		}
	}

	// Graphs
	const TArray<TSharedPtr<FJsonValue>>* GraphsArray;
	if (JsonObject->TryGetArrayField(TEXT("graphs"), GraphsArray) && GraphsArray->Num() > 0)
	{
		Markdown += TEXT("## Graphs\n\n");
		for (const TSharedPtr<FJsonValue>& GraphValue : *GraphsArray)
		{
			TSharedPtr<FJsonObject> GraphObj = GraphValue->AsObject();
			FString GraphName = GraphObj->GetStringField(TEXT("name"));

			const TArray<TSharedPtr<FJsonValue>>* NodesArray;
			if (GraphObj->TryGetArrayField(TEXT("nodes"), NodesArray))
			{
				Markdown += FString::Printf(TEXT("### %s\n\n"), *GraphName);
				Markdown += FString::Printf(TEXT("**Total Nodes:** %d\n\n"), NodesArray->Num());

				// List node types
				TMap<FString, int32> NodeTypeCounts;
				for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray)
				{
					TSharedPtr<FJsonObject> NodeObj = NodeValue->AsObject();
					FString NodeType = NodeObj->GetStringField(TEXT("type"));
					NodeTypeCounts.FindOrAdd(NodeType, 0)++;
				}

				if (NodeTypeCounts.Num() > 0)
				{
					Markdown += TEXT("**Node Types:**\n\n");
					for (const TPair<FString, int32>& Pair : NodeTypeCounts)
					{
						Markdown += FString::Printf(TEXT("- %s: %d\n"), *Pair.Key, Pair.Value);
					}
					Markdown += TEXT("\n");
				}
			}
		}
	}

	// Dependencies
	const TArray<TSharedPtr<FJsonValue>>* DependenciesArray;
	if (JsonObject->TryGetArrayField(TEXT("dependencies"), DependenciesArray) && DependenciesArray->Num() > 0)
	{
		Markdown += TEXT("## Dependencies\n\n");
		int32 Count = FMath::Min(DependenciesArray->Num(), 10); // Limit to first 10
		for (int32 i = 0; i < Count; i++)
		{
			FString Dependency = (*DependenciesArray)[i]->AsString();
			Markdown += FString::Printf(TEXT("- `%s`\n"), *Dependency);
		}
		if (DependenciesArray->Num() > 10)
		{
			Markdown += FString::Printf(TEXT("\n_...and %d more_\n"), DependenciesArray->Num() - 10);
		}
		Markdown += TEXT("\n");
	}

	Markdown += TEXT("---\n\n");
	Markdown += TEXT("_Generated by Blueprint Exporter Plugin_\n");

	return Markdown;
}

// ============================================================================
// Serialization Functions
// ============================================================================

TSharedPtr<FJsonObject> UBlueprintExporterLibrary::SerializeBlueprint(UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		UE_LOG(LogBlueprintExporter, Error, TEXT("SerializeBlueprint: Invalid blueprint"));
		return MakeShareable(new FJsonObject);
	}

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

	// Basic info
	JsonObject->SetStringField(TEXT("name"), Blueprint->GetName());
	JsonObject->SetStringField(TEXT("path"), Blueprint->GetPathName());
	JsonObject->SetStringField(TEXT("class_type"), TEXT("Blueprint"));

	// Parent class
	if (Blueprint->ParentClass)
	{
		JsonObject->SetStringField(TEXT("parent_class"), Blueprint->ParentClass->GetName());
	}

	// Generated class
	if (Blueprint->GeneratedClass)
	{
		JsonObject->SetStringField(TEXT("generated_class"), Blueprint->GeneratedClass->GetName());
	}

	// Graphs
	TArray<TSharedPtr<FJsonValue>> GraphsArray;
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (Graph)
		{
			GraphsArray.Add(MakeShareable(new FJsonValueObject(SerializeGraph(Graph))));
		}
	}

	// Also add function graphs
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph)
		{
			GraphsArray.Add(MakeShareable(new FJsonValueObject(SerializeGraph(Graph))));
		}
	}

	JsonObject->SetArrayField(TEXT("graphs"), GraphsArray);

	// Variables
	JsonObject->SetArrayField(TEXT("variables"), SerializeVariables(Blueprint));

	// Functions
	JsonObject->SetArrayField(TEXT("functions"), SerializeFunctions(Blueprint));

	// Components
	JsonObject->SetArrayField(TEXT("components"), SerializeComponents(Blueprint));

	// Dependencies
	JsonObject->SetArrayField(TEXT("dependencies"), ExtractDependencies(Blueprint));

	return JsonObject;
}

TSharedPtr<FJsonObject> UBlueprintExporterLibrary::SerializeGraph(UEdGraph* Graph)
{
	TSharedPtr<FJsonObject> GraphObject = MakeShareable(new FJsonObject);

	if (!Graph)
	{
		UE_LOG(LogBlueprintExporter, Warning, TEXT("SerializeGraph: Invalid graph"));
		return GraphObject;
	}

	GraphObject->SetStringField(TEXT("name"), Graph->GetName());

	// Serialize nodes
	TArray<TSharedPtr<FJsonValue>> NodesArray;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Node)
		{
			NodesArray.Add(MakeShareable(new FJsonValueObject(SerializeNode(Node))));
		}
	}

	GraphObject->SetArrayField(TEXT("nodes"), NodesArray);

	return GraphObject;
}

TSharedPtr<FJsonObject> UBlueprintExporterLibrary::SerializeNode(UEdGraphNode* Node)
{
	TSharedPtr<FJsonObject> NodeObject = MakeShareable(new FJsonObject);

	if (!Node)
	{
		UE_LOG(LogBlueprintExporter, Warning, TEXT("SerializeNode: Invalid node"));
		return NodeObject;
	}

	NodeObject->SetStringField(TEXT("id"), Node->GetName());
	NodeObject->SetStringField(TEXT("type"), NodeTypeToString(Node));
	NodeObject->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
	NodeObject->SetStringField(TEXT("category"), GetNodeCategory(Node));

	// Position
	TSharedPtr<FJsonObject> PosObject = MakeShareable(new FJsonObject);
	PosObject->SetNumberField(TEXT("x"), Node->NodePosX);
	PosObject->SetNumberField(TEXT("y"), Node->NodePosY);
	NodeObject->SetObjectField(TEXT("position"), PosObject);

	// Pins
	TArray<TSharedPtr<FJsonValue>> PinsArray;
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin)
		{
			PinsArray.Add(MakeShareable(new FJsonValueObject(SerializePin(Pin))));
		}
	}
	NodeObject->SetArrayField(TEXT("pins"), PinsArray);

	// Connected nodes
	TArray<TSharedPtr<FJsonValue>> ConnectionsArray;
	TArray<UEdGraphNode*> ConnectedNodes = GetConnectedNodes(Node);
	for (UEdGraphNode* ConnectedNode : ConnectedNodes)
	{
		ConnectionsArray.Add(MakeShareable(new FJsonValueString(ConnectedNode->GetName())));
	}
	NodeObject->SetArrayField(TEXT("connections"), ConnectionsArray);

	return NodeObject;
}

TSharedPtr<FJsonObject> UBlueprintExporterLibrary::SerializePin(UEdGraphPin* Pin)
{
	TSharedPtr<FJsonObject> PinObject = MakeShareable(new FJsonObject);

	PinObject->SetStringField(TEXT("name"), Pin->GetName());
	PinObject->SetStringField(TEXT("display_name"), Pin->GetDisplayName().ToString());
	PinObject->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
	PinObject->SetStringField(TEXT("type"), PinTypeToString(Pin->PinType));

	// Default value
	if (!Pin->DefaultValue.IsEmpty())
	{
		PinObject->SetStringField(TEXT("default_value"), Pin->DefaultValue);
	}

	// Pin-to-pin connections
	if (Pin->LinkedTo.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> ConnectionsArray;
		for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
		{
			if (LinkedPin && LinkedPin->GetOwningNode())
			{
				TSharedPtr<FJsonObject> ConnectionObj = MakeShareable(new FJsonObject);
				ConnectionObj->SetStringField(TEXT("node_id"), LinkedPin->GetOwningNode()->GetName());
				ConnectionObj->SetStringField(TEXT("node_title"), LinkedPin->GetOwningNode()->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
				ConnectionObj->SetStringField(TEXT("pin_name"), LinkedPin->GetName());
				ConnectionObj->SetStringField(TEXT("pin_display_name"), LinkedPin->GetDisplayName().ToString());
				ConnectionsArray.Add(MakeShareable(new FJsonValueObject(ConnectionObj)));
			}
		}
		PinObject->SetArrayField(TEXT("connected_to"), ConnectionsArray);
	}

	return PinObject;
}

TArray<TSharedPtr<FJsonValue>> UBlueprintExporterLibrary::SerializeVariables(UBlueprint* Blueprint)
{
	TArray<TSharedPtr<FJsonValue>> VariablesArray;

	for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
	{
		TSharedPtr<FJsonObject> VarObject = MakeShareable(new FJsonObject);

		VarObject->SetStringField(TEXT("name"), Variable.VarName.ToString());
		VarObject->SetStringField(TEXT("type"), PinTypeToString(Variable.VarType));
		VarObject->SetStringField(TEXT("category"), Variable.Category.ToString());
		VarObject->SetBoolField(TEXT("is_exposed"), (Variable.PropertyFlags & CPF_ExposeOnSpawn) != 0);

		// Default value
		if (!Variable.DefaultValue.IsEmpty())
		{
			VarObject->SetStringField(TEXT("default_value"), Variable.DefaultValue);
		}

		VariablesArray.Add(MakeShareable(new FJsonValueObject(VarObject)));
	}

	return VariablesArray;
}

TArray<TSharedPtr<FJsonValue>> UBlueprintExporterLibrary::SerializeFunctions(UBlueprint* Blueprint)
{
	TArray<TSharedPtr<FJsonValue>> FunctionsArray;

	for (UEdGraph* FunctionGraph : Blueprint->FunctionGraphs)
	{
		if (!FunctionGraph) continue;

		TSharedPtr<FJsonObject> FuncObject = MakeShareable(new FJsonObject);

		FuncObject->SetStringField(TEXT("name"), FunctionGraph->GetName());

		// Find function entry node to get parameters
		TArray<TSharedPtr<FJsonValue>> ParamsArray;
		for (UEdGraphNode* Node : FunctionGraph->Nodes)
		{
			if (UK2Node_FunctionEntry* EntryNode = Cast<UK2Node_FunctionEntry>(Node))
			{
				for (UEdGraphPin* Pin : EntryNode->Pins)
				{
					if (Pin && Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
					{
						TSharedPtr<FJsonObject> ParamObject = MakeShareable(new FJsonObject);
						ParamObject->SetStringField(TEXT("name"), Pin->GetName());
						ParamObject->SetStringField(TEXT("type"), PinTypeToString(Pin->PinType));
						ParamsArray.Add(MakeShareable(new FJsonValueObject(ParamObject)));
					}
				}
			}
		}

		FuncObject->SetArrayField(TEXT("parameters"), ParamsArray);

		// Include the graph structure
		FuncObject->SetObjectField(TEXT("graph"), SerializeGraph(FunctionGraph));

		FunctionsArray.Add(MakeShareable(new FJsonValueObject(FuncObject)));
	}

	return FunctionsArray;
}

TArray<TSharedPtr<FJsonValue>> UBlueprintExporterLibrary::SerializeComponents(UBlueprint* Blueprint)
{
	TArray<TSharedPtr<FJsonValue>> ComponentsArray;

	// Get components from SimpleConstructionScript
	if (Blueprint->SimpleConstructionScript)
	{
		const TArray<USCS_Node*>& Nodes = Blueprint->SimpleConstructionScript->GetAllNodes();

		for (USCS_Node* Node : Nodes)
		{
			if (Node && Node->ComponentTemplate)
			{
				TSharedPtr<FJsonObject> CompObject = MakeShareable(new FJsonObject);

				CompObject->SetStringField(TEXT("name"), Node->GetVariableName().ToString());
				CompObject->SetStringField(TEXT("class"), Node->ComponentTemplate->GetClass()->GetName());

				ComponentsArray.Add(MakeShareable(new FJsonValueObject(CompObject)));
			}
		}
	}

	return ComponentsArray;
}

TArray<TSharedPtr<FJsonValue>> UBlueprintExporterLibrary::ExtractDependencies(UBlueprint* Blueprint)
{
	TArray<TSharedPtr<FJsonValue>> DependenciesArray;
	TSet<FString> UniqueDependencies;

	// Helper lambda to process a single graph
	auto ProcessGraph = [&UniqueDependencies, &DependenciesArray](UEdGraph* Graph)
	{
		if (!Graph) return;

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node) continue;

			// Check for function calls
			if (UK2Node_CallFunction* CallNode = Cast<UK2Node_CallFunction>(Node))
			{
				if (UClass* FunctionClass = CallNode->FunctionReference.GetMemberParentClass())
				{
					FString ClassPath = FunctionClass->GetPathName();
					if (!ClassPath.IsEmpty() && !UniqueDependencies.Contains(ClassPath))
					{
						UniqueDependencies.Add(ClassPath);
						DependenciesArray.Add(MakeShareable(new FJsonValueString(ClassPath)));
					}
				}
			}

			// Check pins for object references
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object)
				{
					if (UObject* DefaultObject = Pin->DefaultObject)
					{
						FString ObjectPath = DefaultObject->GetPathName();
						if (!ObjectPath.IsEmpty() && !UniqueDependencies.Contains(ObjectPath))
						{
							UniqueDependencies.Add(ObjectPath);
							DependenciesArray.Add(MakeShareable(new FJsonValueString(ObjectPath)));
						}
					}
				}
			}
		}
	};

	// Search through all graph types
	// 1. Event graphs (UbergraphPages)
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		ProcessGraph(Graph);
	}

	// 2. Function graphs
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		ProcessGraph(Graph);
	}

	// 3. Macro graphs
	for (UEdGraph* Graph : Blueprint->MacroGraphs)
	{
		ProcessGraph(Graph);
	}

	// 4. Delegate signature graphs
	for (UEdGraph* Graph : Blueprint->DelegateSignatureGraphs)
	{
		ProcessGraph(Graph);
	}

	return DependenciesArray;
}

// ============================================================================
// Helper Functions
// ============================================================================

FString UBlueprintExporterLibrary::PinTypeToString(const FEdGraphPinType& PinType)
{
	FString TypeString = PinType.PinCategory.ToString();

	if (PinType.PinSubCategoryObject.IsValid())
	{
		TypeString += TEXT("<") + PinType.PinSubCategoryObject->GetName() + TEXT(">");
	}

	if (PinType.IsArray())
	{
		TypeString = TEXT("Array<") + TypeString + TEXT(">");
	}

	return TypeString;
}

FString UBlueprintExporterLibrary::NodeTypeToString(UEdGraphNode* Node)
{
	if (!Node) return TEXT("Unknown");

	if (Cast<UK2Node_Event>(Node)) return TEXT("Event");
	if (Cast<UK2Node_FunctionEntry>(Node)) return TEXT("FunctionEntry");
	if (Cast<UK2Node_CallFunction>(Node)) return TEXT("CallFunction");
	if (Cast<UK2Node_VariableGet>(Node)) return TEXT("VariableGet");
	if (Cast<UK2Node_VariableSet>(Node)) return TEXT("VariableSet");

	return Node->GetClass()->GetName();
}

FString UBlueprintExporterLibrary::GetNodeCategory(UEdGraphNode* Node)
{
	if (UK2Node* K2Node = Cast<UK2Node>(Node))
	{
		return K2Node->GetMenuCategory().ToString();
	}

	return TEXT("");
}

TArray<UEdGraphNode*> UBlueprintExporterLibrary::GetConnectedNodes(UEdGraphNode* Node)
{
	TArray<UEdGraphNode*> ConnectedNodes;

	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (!Pin) continue;

		// Check both input and output pins for bidirectional connection tracking
		for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
		{
			if (LinkedPin && LinkedPin->GetOwningNode())
			{
				ConnectedNodes.AddUnique(LinkedPin->GetOwningNode());
			}
		}
	}

	return ConnectedNodes;
}

// ============================================================================
// Blueprint Change Monitor Implementation
// ============================================================================

void UBlueprintChangeMonitor::StartMonitoring(FOnBlueprintChanged OnChanged)
{
	if (bIsMonitoring)
	{
		UE_LOG(LogBlueprintExporter, Warning, TEXT("Blueprint change monitoring already started"));
		return;
	}

	OnBlueprintChangedDelegate = OnChanged;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	AssetRegistry.OnAssetAdded().AddUObject(this, &UBlueprintChangeMonitor::OnAssetAdded);
	AssetRegistry.OnAssetRemoved().AddUObject(this, &UBlueprintChangeMonitor::OnAssetRemoved);
	AssetRegistry.OnAssetUpdated().AddUObject(this, &UBlueprintChangeMonitor::OnAssetModified);

	bIsMonitoring = true;

	UE_LOG(LogBlueprintExporter, Log, TEXT("Blueprint change monitoring started"));
}

void UBlueprintChangeMonitor::StopMonitoring()
{
	// Early return if not monitoring
	if (!bIsMonitoring)
	{
		return;
	}

	// Check if AssetRegistry module is still loaded before trying to access it
	// During shutdown, modules may be destroyed in any order
	if (!FModuleManager::Get().IsModuleLoaded("AssetRegistry"))
	{
		UE_LOG(LogBlueprintExporter, Warning, TEXT("Blueprint change monitoring cleanup skipped - AssetRegistry module already unloaded"));
		bIsMonitoring = false;
		return;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	AssetRegistry.OnAssetAdded().RemoveAll(this);
	AssetRegistry.OnAssetRemoved().RemoveAll(this);
	AssetRegistry.OnAssetUpdated().RemoveAll(this);

	bIsMonitoring = false;

	UE_LOG(LogBlueprintExporter, Log, TEXT("Blueprint change monitoring stopped"));
}

void UBlueprintChangeMonitor::BeginDestroy()
{
	// Ensure monitoring is stopped before destruction
	StopMonitoring();

	Super::BeginDestroy();
}

void UBlueprintChangeMonitor::OnAssetAdded(const FAssetData& AssetData)
{
	if (AssetData.AssetClassPath == UBlueprint::StaticClass()->GetClassPathName())
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset()))
		{
			OnBlueprintChangedDelegate.ExecuteIfBound(Blueprint);
		}
	}
}

void UBlueprintChangeMonitor::OnAssetRemoved(const FAssetData& AssetData)
{
	// Handle blueprint removal if needed
}

void UBlueprintChangeMonitor::OnAssetModified(const FAssetData& AssetData)
{
	if (AssetData.AssetClassPath == UBlueprint::StaticClass()->GetClassPathName())
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset()))
		{
			OnBlueprintChangedDelegate.ExecuteIfBound(Blueprint);
		}
	}
}

// ============================================================================
// Python Integration Helper
// ============================================================================

class FBlueprintExporterPython
{
public:
	static void Register()
	{
#if defined(WITH_PYTHON) && WITH_PYTHON
		if (!FModuleManager::Get().IsModuleLoaded("PythonScriptPlugin"))
		{
			UE_LOG(LogBlueprintExporter, Log, TEXT("Python plugin not loaded - skipping Python command registration"));
			return;
		}

		TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("BlueprintExporter"));
		if (!Plugin.IsValid())
		{
			UE_LOG(LogBlueprintExporter, Warning, TEXT("Could not find BlueprintExporter plugin for Python path registration"));
			return;
		}

		FString PluginPythonDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Content"), TEXT("Python"));
		PluginPythonDir = FPaths::ConvertRelativePathToFull(PluginPythonDir);
		PluginPythonDir.ReplaceInline(TEXT("\\"), TEXT("\\\\"));

		FString PythonScript = FString::Printf(TEXT(
			"import sys\n"
			"import unreal\n"
			"\n"
			"plugin_python_dir = r'%s'\n"
			"if plugin_python_dir not in sys.path:\n"
			"    sys.path.insert(0, plugin_python_dir)\n"
			"    unreal.log(f'Added to Python path: {plugin_python_dir}')\n"
			"\n"
			"import blueprint_watcher\n"
			"\n"
			"def export_blueprints():\n"
			"    '''Export all blueprints to JSON and Markdown'''\n"
			"    blueprint_watcher.main()\n"
			"\n"
			"unreal.log('Python command registered: export_blueprints()')\n"
		), *PluginPythonDir);

		IPythonScriptPlugin& PythonPlugin = FModuleManager::LoadModuleChecked<IPythonScriptPlugin>("PythonScriptPlugin");
		FPythonCommandEx PythonCommand;
		PythonCommand.ExecutionMode = EPythonCommandExecutionMode::ExecuteFile;
		PythonCommand.Command = PythonScript;
		PythonPlugin.ExecPythonCommandEx(PythonCommand);
#else
		UE_LOG(LogBlueprintExporter, Log, TEXT("Python support not compiled in - skipping Python command registration"));
#endif
	}
};

// ============================================================================
// Module Implementation
// ============================================================================

class FBlueprintExporterModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UE_LOG(LogBlueprintExporter, Log, TEXT("BlueprintExporter module started"));

		if (!IsRunningCommandlet())
		{
			FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
			UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FBlueprintExporterModule::RegisterMenus));

#if defined(WITH_PYTHON) && WITH_PYTHON
			if (FModuleManager::Get().IsModuleLoaded("PythonScriptPlugin"))
			{
				IPythonScriptPlugin& PythonPlugin = FModuleManager::GetModuleChecked<IPythonScriptPlugin>("PythonScriptPlugin");
				PythonPlugin.OnPythonInitialized().AddLambda([]() { FBlueprintExporterPython::Register(); });
			}
#endif
		}
	}

	virtual void ShutdownModule() override
	{
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
		UE_LOG(LogBlueprintExporter, Log, TEXT("BlueprintExporter module shutdown"));
	}

private:
	void RegisterMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(this);

		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
		FToolMenuSection& Section = Menu->AddSection("BlueprintExporter", FText::FromString("Blueprint Exporter"));

		Section.AddMenuEntry(
			"ExportBlueprints",
			FText::FromString("Export Blueprints"),
			FText::FromString("Export all blueprints to JSON and Markdown"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateStatic(&FBlueprintExporterModule::ExecuteExport))
		);
	}
	static void ExecuteExport()
	{
		UE_LOG(LogBlueprintExporter, Log, TEXT("Starting blueprint export from menu..."));

		const UBlueprintExporterSettings* Settings = GetDefault<UBlueprintExporterSettings>();
		FString ProjectDir = FPaths::ProjectDir();
		FString OutputDir = FPaths::Combine(ProjectDir, Settings->OutputDirectory);

		// Export with pretty-printed JSON and markdown generation
		int32 ExportedCount = UBlueprintExporterLibrary::ExportAllBlueprints(OutputDir, Settings->bPrettyPrintJson, Settings->bGenerateMarkdown);

		UE_LOG(LogBlueprintExporter, Log, TEXT("Export complete! Exported %d blueprints to: %s"), ExportedCount, *OutputDir);
	}
};

IMPLEMENT_MODULE(FBlueprintExporterModule, BlueprintExporter)
