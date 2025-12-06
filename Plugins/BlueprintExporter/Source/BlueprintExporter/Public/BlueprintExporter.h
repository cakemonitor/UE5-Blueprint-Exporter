// BlueprintExporter.h
// C++ Plugin for extracting UE5 Blueprint graph data
// Place this in: Plugins/BlueprintExporter/Source/BlueprintExporter/Public/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/Blueprint.h"
#include "Engine/DeveloperSettings.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "BlueprintExporter.generated.h"

// Custom log category for Blueprint Exporter
DECLARE_LOG_CATEGORY_EXTERN(LogBlueprintExporter, Log, All);

/**
 * Project settings for Blueprint Exporter
 */
UCLASS(Config=Editor, DefaultConfig, meta=(DisplayName="Blueprint Exporter"))
class BLUEPRINTEXPORTER_API UBlueprintExporterSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "Export Settings", meta = (DisplayName = "Output Directory"))
	FString OutputDirectory = TEXT("Exported/Blueprints");

	UPROPERTY(Config, EditAnywhere, Category = "Export Settings", meta = (DisplayName = "Generate Markdown Files"))
	bool bGenerateMarkdown = true;

	UPROPERTY(Config, EditAnywhere, Category = "Export Settings", meta = (DisplayName = "Pretty Print JSON"))
	bool bPrettyPrintJson = true;

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};

/**
 * Configuration for blueprint export operations
 */
USTRUCT(BlueprintType)
struct BLUEPRINTEXPORTER_API FBlueprintExportConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Blueprint Exporter")
	FString OutputDirectory = TEXT("Exported/Blueprints");

	UPROPERTY(BlueprintReadWrite, Category = "Blueprint Exporter")
	bool bGenerateMarkdown = true;

	UPROPERTY(BlueprintReadWrite, Category = "Blueprint Exporter")
	bool bPrettyPrintJson = true;
};

/**
 * Blueprint Exporter Library
 * Provides functions to extract blueprint data for Claude Code integration
 */
UCLASS()
class BLUEPRINTEXPORTER_API UBlueprintExporterLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Extract complete blueprint data as JSON string
	 * @param Blueprint - The blueprint to extract data from
	 * @param bPrettyPrint - Whether to format JSON with indentation (default: true)
	 * @return JSON string containing all blueprint information
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Exporter")
	static FString ExtractBlueprintData(UBlueprint* Blueprint, bool bPrettyPrint = true);

	/**
	 * Export blueprint to JSON file
	 * @param Blueprint - The blueprint to export
	 * @param FilePath - Output file path (absolute)
	 * @param bPrettyPrint - Whether to format JSON with indentation (default: true)
	 * @return True if export succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Exporter")
	static bool ExportBlueprintToFile(UBlueprint* Blueprint, const FString& FilePath, bool bPrettyPrint = true);

	/**
	 * Export blueprint to Markdown file
	 * @param Blueprint - The blueprint to export
	 * @param FilePath - Output file path (absolute)
	 * @return True if export succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Exporter")
	static bool ExportBlueprintToMarkdown(UBlueprint* Blueprint, const FString& FilePath);

	/**
	 * Export all project blueprints to directory (JSON and Markdown)
	 * @param OutputDirectory - Directory to export to
	 * @param bPrettyPrint - Whether to format JSON with indentation (default: true)
	 * @param bGenerateMarkdown - Whether to generate Markdown files alongside JSON (default: true)
	 * @return Number of blueprints exported
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Exporter")
	static int32 ExportAllBlueprints(const FString& OutputDirectory, bool bPrettyPrint = true, bool bGenerateMarkdown = true);

	/**
	 * Export all project blueprints using configuration struct
	 * @param Config - Export configuration
	 * @return Number of blueprints exported
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Exporter")
	static int32 ExportAllBlueprintsWithConfig(const FBlueprintExportConfig& Config);

private:
	// Internal serialization and generation functions
	static FString GenerateMarkdownFromJson(TSharedPtr<FJsonObject> JsonObject, UBlueprint* Blueprint);

	// Internal serialization functions
	static TSharedPtr<FJsonObject> SerializeBlueprint(UBlueprint* Blueprint);
	static TSharedPtr<FJsonObject> SerializeGraph(UEdGraph* Graph);
	static TSharedPtr<FJsonObject> SerializeNode(UEdGraphNode* Node);
	static TSharedPtr<FJsonObject> SerializePin(UEdGraphPin* Pin);
	static TArray<TSharedPtr<FJsonValue>> SerializeVariables(UBlueprint* Blueprint);
	static TArray<TSharedPtr<FJsonValue>> SerializeFunctions(UBlueprint* Blueprint);
	static TArray<TSharedPtr<FJsonValue>> SerializeComponents(UBlueprint* Blueprint);
	static TArray<TSharedPtr<FJsonValue>> ExtractDependencies(UBlueprint* Blueprint);

	// Helper functions
	static FString PinTypeToString(const FEdGraphPinType& PinType);
	static FString NodeTypeToString(UEdGraphNode* Node);
	static FString GetNodeCategory(UEdGraphNode* Node);
	static TArray<UEdGraphNode*> GetConnectedNodes(UEdGraphNode* Node);

	// Execution-flow ordering helpers
	static bool IsEntryPointNode(UEdGraphNode* Node);
	static TArray<UEdGraphPin*> GetSortedExecOutputPins(UEdGraphNode* Node);
	static void ExportNodeRecursive(
		UEdGraphNode* Node,
		TSet<UEdGraphNode*>& UnexportedNodes,
		TArray<TSharedPtr<FJsonValue>>& OrderedNodesArray
	);
};
