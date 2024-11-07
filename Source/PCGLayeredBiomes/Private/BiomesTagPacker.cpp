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



#include "BiomesTagPacker.h"

#include "BiomesSpawnManager.h"
#include "PCGComponent.h"
#include "Engine/StaticMesh.h"
#include "PCGContext.h"
#include "MeshSelectors/PCGMeshSelectorBase.h"

void UBiomesTagPacker::PackInstances_Implementation(FPCGContext& Context, const UPCGSpatialData* InSpatialData,
	const FPCGMeshInstanceList& InstanceList, FPCGPackedCustomData& OutPackedCustomData) const
{
	if (const auto* Actor = Context.SourceComponent->GetOriginalComponent()->GetOwner())
	{
		if (const auto* Manager = Actor->GetComponentByClass<UBiomesSpawnManager>())
		{
			UBiomesSpawnManager::FPackedTagsEntry TagsEntry;
			if (auto* Info = Manager->FindActorInfoByMesh(InstanceList.Descriptor.StaticMesh, TagsEntry))
			{
				if (!Info->UserData)
				{
					return;
				}

				TArray<UBiomesSpawnManager::FPackedTagsEntry> TypedData;
				TypedData.Init(TagsEntry, InstanceList.Instances.Num());

				constexpr auto FloatsPerTag = sizeof(UBiomesSpawnManager::FPackedTagsEntry) / sizeof(float);
				OutPackedCustomData.NumCustomDataFloats = FloatsPerTag;
				
				const auto NumFloats = TypedData.Num() * FloatsPerTag;
				OutPackedCustomData.CustomData.SetNum(NumFloats);
				FMemory::Memcpy(&OutPackedCustomData.CustomData[0], &TypedData[0], NumFloats * sizeof(float));
			}
		}
	}
}
