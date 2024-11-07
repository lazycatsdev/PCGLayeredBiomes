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



#include "Graph/PCGGetAttributes.h"

#include "BiomesPCGUtils.h"
#include "PCGContext.h"
#include "PCGParamData.h"
#include "PCGPin.h"

#define LOCTEXT_NAMESPACE "PCGGetAttributesSettings"

TArray<FPCGPinProperties> UPCGGetAttributesSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> Properties;
	Properties.Emplace(TEXT("Params"), EPCGDataType::Param);
	return Properties;
}

TArray<FPCGPinProperties> UPCGGetAttributesSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;

	for (const auto& Attribute: Attributes)
	{
		PinProperties.Emplace(Attribute, EPCGDataType::Param, true, false);
	}

	return PinProperties;
}

FPCGElementPtr UPCGGetAttributesSettings::CreateElement() const
{
	return MakeShared<FPCGGetAttributes>();
}

bool FPCGGetAttributes::ExecuteInternal(FPCGContext* Context) const
{
	const auto* Settings = Context->GetInputSettings<UPCGGetAttributesSettings>();
	check(Settings);
	
	const auto& InputParamsData = Context->InputData.GetParamsByPin("Params");
	for (const auto& InputData: InputParamsData)
	{
		const auto& InputParams = Cast<UPCGParamData>(InputData.Data);

		for (const auto& AttributeName: Settings->Attributes)
		{
			const auto* InMetadata = InputParams->ConstMetadata(); 

			const auto* InAttribute = InMetadata->GetConstAttribute(AttributeName);
			if (!InAttribute)
			{
				return true;
			}
			
			TObjectPtr<UPCGParamData> OutData = NewObject<UPCGParamData>();

			auto CreateAttribute = [this, Context, AttributeName, InAttribute, &OutData](auto DummyValue) -> bool
			{
				using AttributeType = decltype(DummyValue);
			
				const auto* TypedAttribute = static_cast<const FPCGMetadataAttribute<AttributeType>*>(InAttribute);
				AttributeType Value = TypedAttribute->GetValue(0);
			
				FPCGMetadataAttribute<AttributeType>* NewAttribute = static_cast<FPCGMetadataAttribute<AttributeType>*>(
					OutData->Metadata->CreateAttribute<AttributeType>(AttributeName, Value, /*bAllowInterpolation=*/false, /*bOverrideParent=*/false));
			
				if (!NewAttribute)
				{
					PCGE_LOG(Error, GraphAndLog, FText::Format(LOCTEXT("ErrorCreatingTargetAttribute", "Error while creating target attribute '{0}'"), FText::FromName(AttributeName)));
					return true;
				}
			
				NewAttribute->SetValue(OutData->Metadata->AddEntry(), Value);
			
				return true;
			};
			
			if (PCGMetadataAttribute::CallbackWithRightType(InAttribute->GetTypeId(), MoveTemp(CreateAttribute)))
			{
				Context->OutputData.TaggedData.Add({OutData, {}, AttributeName, false});
			}
		}
	}

	return true;
}

TArray<FPCGPinProperties> UPCGGetAllAttributesFromSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> Properties;
	Properties.Emplace(TEXT("Params"), EPCGDataType::Param);
	return Properties;
}

TArray<FPCGPinProperties> UPCGGetAllAttributesFromSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> Properties;
	Properties.Emplace(TEXT("Params"), EPCGDataType::Param, true, false);
	Properties.Emplace(TEXT("Success"), EPCGDataType::Param, true, false);
	return Properties;
}

FPCGElementPtr UPCGGetAllAttributesFromSettings::CreateElement() const
{
	return MakeShared<FPCGGetAllAttributesFrom>();
}

bool FPCGGetAllAttributesFrom::ExecuteInternal(FPCGContext* Context) const
{
	const auto* Settings = Context->GetInputSettings<UPCGGetAllAttributesFromSettings>();
	check(Settings);
	
	const auto& InputParamsData = Context->InputData.GetParamsByPin("Params");

	const bool IsSuccess = InputParamsData.IsValidIndex(Settings->DataIndex);
	const TObjectPtr<UPCGParamData> SuccessData = NewObject<UPCGParamData>();
	UBiomesPCGUtils::CreateAndSetAttribute(NAME_None, SuccessData->Metadata, IsSuccess);
	Context->OutputData.TaggedData.Add({SuccessData, {}, "Success", false});

	if (IsSuccess)
	{
		const auto& Params = InputParamsData[Settings->DataIndex];
		Context->OutputData.TaggedData.Add({Params.Data, {}, "Params", false});
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
