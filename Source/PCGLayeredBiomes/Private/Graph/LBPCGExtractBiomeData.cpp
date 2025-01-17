/*
 * Copyright (c) 2024 LazyCatsDev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */



#include "Graph/LBPCGExtractBiomeData.h"

#include "LBBiomesPCGUtils.h"
#include "LBBiomesSpawnManager.h"
#include "PCGContext.h"
#include "PCGParamData.h"
#include "PCGPin.h"
#include "Biomes/LBBiomesSettings.h"
#include "Biomes/Layers/LBBaseBiomeLayer.h"

#if WITH_EDITOR
#include "Helpers/PCGDynamicTrackingHelpers.h"
#endif

#define LOCTEXT_NAMESPACE "PCGSetWeightedMeshFromSet"

TArray<FPCGPinProperties> ULBPCGExtractBiomeData::InputPinProperties() const
{
	TArray<FPCGPinProperties> Properties;
	Properties.Emplace(TEXT("Biome"), EPCGDataType::Param);
	return Properties;
}

TArray<FPCGPinProperties> ULBPCGExtractBiomeData::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	
	PinProperties.Emplace("Layers", EPCGDataType::Param);
	PinProperties.Emplace("Biome Settings", EPCGDataType::Param, true, false);

	return PinProperties;
}

FPCGElementPtr ULBPCGExtractBiomeData::CreateElement() const
{
	return MakeShared<FLBPCGExtractBiomeData>();
}

bool FLBPCGExtractBiomeData::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGExecuteLayers::Execute);

	const auto* Settings = Context->GetInputSettings<ULBPCGExtractBiomeData>();
	check(Settings);

	FName Biome;

	const auto& Params = Context->InputData.GetParamsByPin("Biome");
	if (Params.IsEmpty())
	{
		return true;
	}
	
	const auto& Param = Params.Last();
	if (const auto* BiomeParam = Cast<UPCGParamData>(Param.Data))
	{
		auto* BiomeAttribute = static_cast<const FPCGMetadataAttribute<FName>*>(BiomeParam->ConstMetadata()->GetConstAttribute("Biome"));
		Biome = BiomeAttribute->GetValue(0);
	}
	else
	{
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("InvalidInputData", "Biome pin is not Param (Attribute Set) pin!"));
		return true;
	}

	const auto* Manager = ULBBiomesSpawnManager::GetManager(Context->SourceComponent.Get());
	if (!Manager)
	{
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("NoActorsManager", "Source Actor has no ULBBiomesSpawnManager component"));
		return true;		
	}
	
	if (auto* BiomeSettings = Manager->FindSettings(Biome))
	{
		{
			TObjectPtr<UPCGParamData> OutData = NewObject<UPCGParamData>();
			PCGMetadataEntryKey MetadataKey = OutData->Metadata->AddEntry();
			for (TFieldIterator<const FProperty> FieldIt(BiomeSettings->StaticStruct(), EFieldIterationFlags::IncludeSuper); FieldIt; ++FieldIt)
			{
				const FString FieldName = BiomeSettings->StaticStruct()->GetAuthoredNameForField(*FieldIt);
				const FName AttributeName(FieldName);

				OutData->Metadata->SetAttributeFromDataProperty(AttributeName, MetadataKey, BiomeSettings, *FieldIt, true);
			}
		
			Context->OutputData.TaggedData.Add({OutData, {}, "Biome Settings", false});
		}
		
		for (const auto& Layer: BiomeSettings->Layers)
		{
			if (!Layer)
			{
				continue;
			}
			
			TObjectPtr<UPCGParamData> OutData = NewObject<UPCGParamData>();
			PCGMetadataEntryKey MetadataKey = OutData->Metadata->AddEntry();
			for (const FProperty* Property = Layer->GetClass()->PropertyLink; Property != nullptr; Property = Property->PropertyLinkNext)
			{
				OutData->Metadata->SetAttributeFromProperty(Property->GetFName(), MetadataKey, Layer.Get(), Property, true);
			}

			Context->OutputData.TaggedData.Add({OutData, {}, "Layers", false});
		}
	}
	else
	{
		PCGE_LOG(Warning, GraphAndLog, LOCTEXT("InvalidBiome", "Settings doesn't have biome"));
		return true;
	}

	// Register dynamic tracking
#if WITH_EDITOR
	FPCGDynamicTrackingHelper::AddSingleDynamicTrackingKey(Context, FPCGSelectionKey::CreateFromPath(Manager->GetBiomesSoftPath()), /*bIsCulled=*/false);
#endif // WITH_EDITOR
	
	return true;
}

void FLBPCGExtractBiomeData::GetDependenciesCrc(const FPCGDataCollection& InInput, const UPCGSettings* InSettings,
	UPCGComponent* InComponent, FPCGCrc& OutCrc) const
{
	FPCGCrc Crc;
	FPCGPointProcessingElementBase::GetDependenciesCrc(InInput, InSettings, InComponent, Crc);

	const auto* Manager = ULBBiomesSpawnManager::GetManager(InComponent);
	if (!Manager)
	{
		OutCrc = Crc;
		return;		
	}

	Crc.Combine(Manager->GetBiomesCrc());
	
	OutCrc = Crc;
}

#undef LOCTEXT_NAMESPACE
