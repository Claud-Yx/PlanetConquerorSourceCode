/* Copyright (C) 2021 Victoria Lyons - All Rights Reserved*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ColorizerSettings.generated.h"

USTRUCT( BlueprintType )
struct FPathFolderColor
{
	GENERATED_BODY()

	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "FolderColorizer" )
	FString Path;
	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "FolderColorizer" )
	FLinearColor FolderColor;
	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "FolderColorizer", meta = ( DisplayName = "Affect Subfolders" ) )
	bool bAffectSubFolders;

	FPathFolderColor()
	{
		Path = "/Game";
		//Just a nice default color to start
		FolderColor = FLinearColor( 0.25f, 0.6f, 0.9f, 1.0f );
		bAffectSubFolders = false;
	}
};

USTRUCT( BlueprintType )
struct FKeywordFolderColor
{
	GENERATED_BODY()

	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Folder Colorizer" )
	FString Keyword;
	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Folder Colorizer" )
	FLinearColor FolderColor;
	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Folder Colorizer", meta = ( DisplayName = "Affect Subfolders" ) )
	bool bAffectSubFolders;

	FKeywordFolderColor()
	{
		//Just a nice default color to start
		FolderColor = FLinearColor( 0.25f, 0.6f, 0.9f, 1.0f );
		bAffectSubFolders = false;
	}
};

UENUM( BlueprintType )
enum EColorizerMode
{
	ColorByPath UMETA( DisplayName = "Color by Path" ),
	ColorByKeyword UMETA( DisplayName = "Color by Keyword" ),
};

UCLASS( config = FolderColorizer )
class FOLDERCOLORIZER_API UColorizerSettings : public UObject
{
	GENERATED_BODY()

	public:

	UColorizerSettings();

	UPROPERTY( EditAnywhere, BlueprintReadWrite, config, Category = "Folder Colorizer", meta = ( DisplayName = "Use Colorizer Colors Only", Tooltip = "If checked, the colors of folders not listed in the colorizer will be erased" ) )
	bool bUseColorizerColorsOnly = true;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, config, Category = "Folder Colorizer", meta = ( DisplayName = "Color Mode" ) )
	TEnumAsByte<EColorizerMode> ColorMode = ColorByPath;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, config, Category = "Folder Colorizer", meta = ( DisplayName = "Folder Colors By Path" ) )
	TArray<FPathFolderColor> FolderColorsByPath;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, config, Category = "Folder Colorizer", meta = ( DisplayName = "Folder Colors By Keyword" ) )
	TArray<FKeywordFolderColor> FolderColorsByKeyword;
};
