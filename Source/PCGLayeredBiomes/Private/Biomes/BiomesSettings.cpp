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



#include "Biomes/BiomesSettings.h"

#include "BiomesPCGUtils.h"
#include "Biomes/PCGBiomesBaseFilter.h"
#include "Serialization/ArchiveCrc32.h"

bool UBiomesData::DetectBiome(const FPCGPoint& Point, const UPCGMetadata* Metadata, FName& OutBiome, int& OutPriority) const
{
	static const FName PriorityName = "BiomePriority";
	static const FName BiomeName = "Biome";

	OutPriority = UBiomesPCGUtils::GetInteger32Attribute(Point, Metadata, PriorityName);
	OutBiome = UBiomesPCGUtils::GetNameAttribute(Point, Metadata, BiomeName);
	
	for (const auto& Biome: Biomes)
	{
		bool AllMatched = true;
		for (const auto& Filter: Biome.Filters)
		{
			if (!Filter || !Filter->Filter(Point, Metadata))
			{
				AllMatched = false;
				break;
			}
		}
		if (AllMatched)
		{
			// First match is the best one, but we should take to account value from point
			if (Biome.Priority < OutPriority)
			{
				OutBiome = Biome.Name;
				OutPriority = Biome.Priority;
			}
			break;
		}
	}
	return !OutBiome.IsNone();
}


FBiomeSettings_Named::FBiomeSettings_Named(FName InName, FBiomeSettings InSettings)
	: FBiomeSettings(InSettings)
	, Name(InName)
{
}

FPCGCrc UBiomesSettings::ComputeCrc()
{
	FPCGCrc Result(Biomes.Num());
	for (auto& Item : Biomes)
	{
		FArchiveCrc32 Ar;
		Ar << Item.Key;
		Ar << Item.Value;
		Result.Combine(Ar.GetCrc());
	}
	return Result;
}

UBiomesData* UBiomesSettings::Prepare() const
{
	auto* Result = NewObject<UBiomesData>();
	
	Result->Biomes.Reserve(Biomes.Num());

	for (const auto& [Name, Biome]: Biomes)
	{
		Result->Biomes.Emplace(Name, Biome);
	}

	Result->Biomes.StableSort([](const auto& A, const auto& B) { return A.Priority < B.Priority; } );

	return Result;
}

const FBiomeSettings* UBiomesSettings::FindSettings(FName Name) const
{
	return Biomes.Find(Name);
}

#if WITH_EDITOR
static FLinearColor GetNextColor()
{
	constexpr int NumPredefinedColors = 30;
	constexpr FLinearColor PredefinedColors[NumPredefinedColors] = {
		FLinearColor(0.247059, 0.705882, 0.988235, 1.000000),
		FLinearColor(0.027451, 0.949020, 0.603922, 1.000000),
		FLinearColor(0.992157, 0.486275, 0.129412, 1.000000),
		FLinearColor(0.823529, 0.168627, 0.788235, 1.000000),
		FLinearColor(0.992157, 0.250980, 0.501961, 1.000000),
		FLinearColor(0.000000, 0.956863, 0.854902, 1.000000),
		FLinearColor(1.000000, 0.992157, 0.392157, 1.000000),
		FLinearColor(0.984314, 0.105882, 0.764706, 1.000000),
		FLinearColor(0.376471, 0.219608, 0.815686, 1.000000),
		FLinearColor(0.890196, 0.725490, 0.203922, 1.000000),
		FLinearColor(0.207843, 0.956863, 0.964706, 1.000000),
		FLinearColor(0.047059, 0.603922, 0.988235, 1.000000),
		FLinearColor(0.000000, 0.854902, 0.419608, 1.000000),
		FLinearColor(0.952941, 0.368627, 0.160784, 1.000000),
		FLinearColor(0.788235, 0.164706, 0.701961, 1.000000),
		FLinearColor(0.988235, 0.156863, 0.443137, 1.000000),
		FLinearColor(0.109804, 0.827451, 0.788235, 1.000000),
		FLinearColor(0.988235, 0.949020, 0.258824, 1.000000),
		FLinearColor(0.972549, 0.098039, 0.658824, 1.000000),
		FLinearColor(0.298039, 0.168627, 0.721569, 1.000000),
		FLinearColor(0.803922, 0.635294, 0.203922, 1.000000),
		FLinearColor(0.117647, 0.878431, 0.913725, 1.000000),
		FLinearColor(0.086275, 0.443137, 0.992157, 1.000000),
		FLinearColor(0.000000, 0.666667, 0.321569, 1.000000),
		FLinearColor(0.921569, 0.258824, 0.078431, 1.000000),
		FLinearColor(0.670588, 0.156863, 0.576471, 1.000000),
		FLinearColor(0.992157, 0.105882, 0.298039, 1.000000),
		FLinearColor(0.039216, 0.752941, 0.709804, 1.000000),
		FLinearColor(0.996078, 0.901961, 0.372549, 1.000000),
		FLinearColor(0.905882, 0.086275, 0.600000, 1.000000)
	};
	static int ColorIndex = 0;
	if (ColorIndex < NumPredefinedColors)
	{
		return PredefinedColors[++ColorIndex];
	}
	return FLinearColor::MakeRandomColor();
}

void UBiomesSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if(PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UBiomesData, Biomes))
	{
		for (auto& Pair : Biomes)
		{
			if (Pair.Value.DebugColor == FLinearColor::Transparent)
			{
				Pair.Value.DebugColor = GetNextColor();
			}
		}
	}
}
#endif
