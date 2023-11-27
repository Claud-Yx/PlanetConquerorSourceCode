/* Copyright (C) 2021 Victoria Lyons - All Rights Reserved*/

#include "FolderColorizer.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "ColorizerSettings.h"
#include "HAL/FileManagerGeneric.h"
#include "ContentBrowserModule.h"


#define LOCTEXT_NAMESPACE "FFolderColorizerModule"

void FFolderColorizerModule::StartupModule()
{
	//Add folder colors to project settings
	RegisterSettings();

	//Add "Refresh Folder Colors" button to content browser context menu
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>( TEXT( "ContentBrowser" ) );
	TArray<FContentBrowserMenuExtender_SelectedPaths>& PathViewExtenderDelegates = ContentBrowserModule.GetAllPathViewContextMenuExtenders();
	TArray<FContentBrowserMenuExtender_SelectedPaths>& AssetViewExtenderDelegates = ContentBrowserModule.GetAllAssetContextMenuExtenders();
	PathViewExtenderDelegates.Add( FContentBrowserMenuExtender_SelectedPaths::CreateRaw( this, &FFolderColorizerModule::UpdateColorsExtender ) );
	AssetViewExtenderDelegates.Add( FContentBrowserMenuExtender_SelectedPaths::CreateRaw( this, &FFolderColorizerModule::UpdateColorsExtender ) );

	//Initial refresh and update of folder colors
	ClearFolderColors();
	UpdateFolderColors();
}

void FFolderColorizerModule::ShutdownModule()
{
	UnregisterSettings();
}

void FFolderColorizerModule::RegisterSettings()
{
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>( "Settings" );

	if ( SettingsModule != nullptr )
	{
		ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings( "Project", "Plugins", "FolderColorizer",
			LOCTEXT( "FolderColorizerName", "Folder Colorizer" ),
			LOCTEXT( "FolderColorizerDescription", "Customize folder appearances" ),
			GetMutableDefault<UColorizerSettings>() );

		if ( SettingsSection != nullptr )
		{
			SettingsSection->OnModified().BindStatic( &FFolderColorizerModule::HandleSettingsModified );
		}
	}
	return;

}

void FFolderColorizerModule::UnregisterSettings()
{
	if ( ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>( "Settings" ) )
	{
		SettingsModule->UnregisterSettings( "Project", "Plugins", "FolderColorizer" );
	}
}

bool FFolderColorizerModule::HandleSettingsModified()
{
	ClearFolderColors();
	UpdateFolderColors();
	return true;
}

TSharedRef<FExtender> FFolderColorizerModule::UpdateColorsExtender( const TArray<FString>& Path )
{
	TSharedPtr<FExtender> ColorizerExtension = MakeShareable( new FExtender() );
	ColorizerExtension->AddMenuExtension( "NewFolder", EExtensionHook::After, TSharedPtr<FUICommandList>(), FMenuExtensionDelegate::CreateStatic( &FFolderColorizerModule::AddExtension ) );
	return ColorizerExtension.ToSharedRef();
}

void FFolderColorizerModule::AddExtension( class FMenuBuilder& MenuBuilder )
{
	MenuBuilder.AddMenuEntry(
		FText::FromString( "Refresh Folder Colors" ),
		FText::FromString( "Refresh Folder Colors" ),
		FSlateIcon(),
		FUIAction( FExecuteAction::CreateStatic( &FFolderColorizerModule::UpdateFolderColors ) )
	);
}

void FFolderColorizerModule::UpdateFolderColors()
{
	UColorizerSettings* Settings = GetMutableDefault<UColorizerSettings>();

	if ( Settings != nullptr )
	{
		if ( Settings->ColorMode == EColorizerMode::ColorByPath )
		{
			TArray<FPathFolderColor> PathFolderColorSettings = Settings->FolderColorsByPath;
			UpdateFolderColorsByPath( PathFolderColorSettings );
		}
		else if ( Settings->ColorMode == EColorizerMode::ColorByKeyword )
		{
			TArray<FKeywordFolderColor> KeywordFolderColorSettings = Settings->FolderColorsByKeyword;
			UpdateFolderColorsByKeyword( KeywordFolderColorSettings );
		}
		else return;
	}

	return;
}

void FFolderColorizerModule::UpdateFolderColorsByPath( TArray<FPathFolderColor> FoldersSettings )
{
	//Sort by how many folders deep each entry is so that higher up folders don't overwrite lower ones
	SortFolderColorInfo( FoldersSettings );

	for ( FPathFolderColor FolderSettings : FoldersSettings )
	{
		FString OriginalFolder = FolderSettings.Path;
		FPaths::NormalizeDirectoryName( OriginalFolder );

		if ( FolderSettings.bAffectSubFolders )
		{

			FString FullPath = OriginalFolder;
			FullPath = FPaths::ProjectContentDir() + OriginalFolder;
			FullPath = FullPath.Replace( TEXT( "/Game/" ), TEXT( "" ), ESearchCase::CaseSensitive );

			TArray<FString> Subfolders;
			FFileManagerGeneric::Get().FindFilesRecursive( Subfolders, *FullPath, TEXT( "*" ), false, true, true );

			TArray<FString> PathsToColor;
			PathsToColor.Add( OriginalFolder );

			for ( FString Folder : Subfolders )
			{
				int32 ChopIndex = Folder.Find( "/Content/" );
				Folder = Folder.RightChop( ChopIndex );
				Folder = Folder.Replace( TEXT( "/Content/" ), TEXT( "/Game/" ), ESearchCase::CaseSensitive );
				PathsToColor.Add( Folder );
			}

			for ( FString Path : PathsToColor )
			{
				SetFolderColor( Path, FolderSettings.FolderColor );
			}

		}
		else
		{
			SetFolderColor( OriginalFolder, FolderSettings.FolderColor );
		}
	}

	return;
}

void FFolderColorizerModule::UpdateFolderColorsByKeyword( TArray<FKeywordFolderColor> FoldersSettings )
{
	/*Sort keywords by length. If a keyword contains another keyword,
	a folder's name should be compared against the longer one first. */
	SortFolderColorInfo( FoldersSettings );

	FString StartingPath = FPaths::ProjectContentDir().Replace( TEXT( "/Game/" ), TEXT( "" ), ESearchCase::CaseSensitive );
	TArray<FString> Subfolders;
	SetKeywordFolderColorsRecursive( Subfolders, *StartingPath, TEXT( "*" ), AssetViewUtils::GetDefaultColor(), false, FoldersSettings );
}

void FFolderColorizerModule::SetKeywordFolderColorsRecursive( TArray<FString>& FileNames, const TCHAR* StartDirectory, const TCHAR* Filename,
	FLinearColor CurrentColor, bool bAffectSubfolders, TArray<FKeywordFolderColor> KeywordColors )
{
	FString CurrentSearch = FString( StartDirectory ) / Filename;
	TArray<FString> Result;
	FFileManagerGeneric::Get().FindFiles( Result, *CurrentSearch, false, true );

	for ( int32 i = 0; i < Result.Num(); i++ )
	{
		FileNames.Add( FString( StartDirectory ) / Result[i] );
	}

	TArray<FString> SubDirs;
	FString RecursiveDirSearch = FString( StartDirectory ) / TEXT( "*" );
	FFileManagerGeneric::Get().FindFiles( SubDirs, *RecursiveDirSearch, false, true );

	for ( int32 SubDirIdx = 0; SubDirIdx < SubDirs.Num(); SubDirIdx++ )
	{
		/*Reformat the path so its color can be set*/
		FString SubDir = FString( StartDirectory ) / SubDirs[SubDirIdx];
		FString PathToColor = SubDir;
		int32 ChopIndex = PathToColor.Find( "/Content/" );
		PathToColor = PathToColor.RightChop( ChopIndex );
		PathToColor = PathToColor.Replace( TEXT( "/Content/" ), TEXT( "/Game/" ), ESearchCase::CaseSensitive );

		bool bFoundKeywordMatch = false;
		FLinearColor NewColor;
		bool bNewAffectSubfolders;

		/*Check if the folder contains a keyword*/
		for ( FKeywordFolderColor KeywordColor : KeywordColors )
		{
			if ( SubDirs[SubDirIdx].Contains( KeywordColor.Keyword ) )
			{
				SetFolderColor( PathToColor, KeywordColor.FolderColor );
				bFoundKeywordMatch = true;
				if ( KeywordColor.bAffectSubFolders )
				{
					NewColor = KeywordColor.FolderColor;
					bNewAffectSubfolders = KeywordColor.bAffectSubFolders;
				}
				else
				{
					NewColor = CurrentColor;
					bNewAffectSubfolders = bAffectSubfolders;
				}
				bFoundKeywordMatch = true;
				break;
			}
		}

		/* No keyword match was found.  Check if the folder should be colored the color of its' parent instead or set to default*/
		if ( !bFoundKeywordMatch && bAffectSubfolders )
		{
			SetFolderColor( PathToColor, CurrentColor );
			NewColor = CurrentColor;
			bNewAffectSubfolders = bAffectSubfolders;
		}
		else if ( !bFoundKeywordMatch )
		{
			NewColor = AssetViewUtils::GetDefaultColor();
			bNewAffectSubfolders = false;
		}

		SetKeywordFolderColorsRecursive( FileNames, *SubDir, Filename, NewColor, bNewAffectSubfolders, KeywordColors );
	}
}

void FFolderColorizerModule::ClearFolderColors()
{
	/*If the user wants to be able to use folder colors other than those set in the Folder
	Colorizer, then the color entries should not be cleared. */
	UColorizerSettings* Settings = GetMutableDefault<UColorizerSettings>();
	if ( !Settings->bUseColorizerColorsOnly || !FPaths::FileExists( GEditorPerProjectIni ) )
	{
		return;
	}

	TArray< FString > Section;
	GConfig->GetSection( TEXT( "PathColor" ), Section, GEditorPerProjectIni );

	for ( int32 SectionIndex = 0; SectionIndex < Section.Num(); SectionIndex++ )
	{
		FString EntryStr = Section[SectionIndex];
		EntryStr.TrimStartInline();

		FString PathStr;
		FString ColorStr;
		if ( EntryStr.Split( TEXT( "=" ), &PathStr, &ColorStr ) )
		{
			//Saving the path as the default color will remove it instead
			TSharedPtr<FLinearColor> DefaultColor = MakeShareable( new FLinearColor() );;
			*DefaultColor.Get() = AssetViewUtils::GetDefaultColor();
			AssetViewUtils::SaveColor( PathStr, DefaultColor, true );

		}
	}
	return;
}

void FFolderColorizerModule::SetFolderColor( FString FolderPath, FLinearColor NewColor )
{
	TSharedPtr<FLinearColor> Color = AssetViewUtils::LoadColor( FolderPath );
	if ( !Color.IsValid() )
	{
		Color = MakeShareable( new FLinearColor() );
	}
	*Color.Get() = NewColor;
	AssetViewUtils::SaveColor( FolderPath, Color );
}

void FFolderColorizerModule::SortFolderColorInfo( TArray<FPathFolderColor>& Folders )
{
	TArray<FPathFolderColor> SortedFolders;
	TMultiMap<int32, int32> DepthToIndices;
	int32 MaxDepth = 1;

	//Find how many folders deep each path is
	for ( int i = 0; i < Folders.Num(); i++ )
	{
		int32 Depth = GetFolderDepth( Folders[i].Path );
		DepthToIndices.Add( Depth, i );
		if ( Depth > MaxDepth )
		{
			MaxDepth = Depth;
		}
	}

	//Sort by the depth found
	for ( int i = 0; i <= MaxDepth; i++ )
	{
		TArray<int32> FolderIndicies;
		DepthToIndices.MultiFind( i, FolderIndicies, true );

		for ( int32 Index : FolderIndicies )
		{
			SortedFolders.Add( Folders[Index] );
		}
	}

	Folders = SortedFolders;

}

void FFolderColorizerModule::SortFolderColorInfo( TArray<FKeywordFolderColor>& Keywords )
{
	TArray<FKeywordFolderColor> SortedKeywords;
	TMultiMap<int32, int32> LengthToIndicies;
	int32 MaxLength = 1;

	for ( int i = 0; i < Keywords.Num(); i++ )
	{
		int32 Length = Keywords[i].Keyword.Len();
		LengthToIndicies.Add( Length, i );
		if ( Length > MaxLength )
		{
			MaxLength = Length;
		}
	}

	for ( int i = MaxLength; i > 0; i-- )
	{
		TArray<int32> KeywordIndices;
		LengthToIndicies.MultiFind( i, KeywordIndices, true );

		for ( int32 Index : KeywordIndices )
		{
			SortedKeywords.Add( Keywords[Index] );
		}
	}
	Keywords = SortedKeywords;
}

int FFolderColorizerModule::GetFolderDepth( FString Path )
{
	int32 Depth = 0;
	TArray<TCHAR> Chars = Path.GetCharArray();
	for ( TCHAR Char : Chars )
	{
		if ( Char == *"/" )
		{
			Depth++;
		}
	}
	return Depth;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE( FFolderColorizerModule, FolderColorizer )