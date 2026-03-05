#include "MarkdownAssetFactory.h"
#include "MarkdownAsset.h"

UMarkdownAssetFactory::UMarkdownAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UMarkdownAsset::StaticClass();
}

UObject* UMarkdownAssetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UMarkdownAsset>(InParent, InClass, InName, Flags | RF_Transactional);
}
