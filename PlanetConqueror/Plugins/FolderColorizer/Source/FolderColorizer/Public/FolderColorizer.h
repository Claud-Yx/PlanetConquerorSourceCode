/* Copyright (C) 2021 Victoria Lyons - All Rights Reserved*/
#pragma once

#include "CoreMinimal.h"
#include "AssetViewUtils.h"

class FFolderColorizerModule : public IModuleInterface
{

public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:

	/*Adds the Folder Colorizer to the project settings*/
	static void RegisterSettings();
	static void UnregisterSettings();
	static bool HandleSettingsModified();


	/* Adds a "Refresh Folder Colors" button to the content browser right click menu */
	TSharedRef<FExtender> UpdateColorsExtender( const TArray<FString>& Path );
	static void AddExtension( class FMenuBuilder& MenuBuilder );

	/* Refreshes folder colors */
	static void UpdateFolderColors();
	static void UpdateFolderColorsByPath( TArray<FPathFolderColor> FoldersSettings );
	static void UpdateFolderColorsByKeyword( TArray<FKeywordFolderColor> FoldersSettings );
	static void ClearFolderColors();
	static void SetFolderColor( FString FolderPath, FLinearColor NewColor );
	static void SetKeywordFolderColorsRecursive( TArray<FString>& FileNames, const TCHAR* StartDirectory,
		const TCHAR* Filename, FLinearColor CurrentColor, bool bAffectSubfolders, TArray<FKeywordFolderColor> KeywordColors );

	/*Used for sorting input paths by how many folders deep each is*/
	static void SortFolderColorInfo( TArray<FPathFolderColor>& Folders );
	static void SortFolderColorInfo( TArray<FKeywordFolderColor>& Keywords );
	static int GetFolderDepth( FString Path );
};
