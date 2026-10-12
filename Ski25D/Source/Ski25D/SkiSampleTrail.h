#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkiSampleTrail.generated.h"

class UProceduralMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;

/** Deterministic, editor-visible scenery. All interactive surfaces stay in the X-Z plane. */
UCLASS()
class SKI25D_API ASkiSampleTrail : public AActor
{
    GENERATED_BODY()
public:
    ASkiSampleTrail();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    UFUNCTION(BlueprintPure, Category="Ski Trail")
    static float HeightAt(float X);
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Ski Trail")
    void RebuildBackdrop();
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Ski Trail")
    void RebuildCourse();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ski Trail") int32 CourseIndex=0;
    UFUNCTION(BlueprintCallable, Category="Ski Trail") static void SetActiveCourse(int32 Index);
    UFUNCTION(BlueprintPure, Category="Ski Trail") static int32 GetActiveCourse();
    UFUNCTION(BlueprintPure, Category="Ski Trail") static float GetFinishX();
    static FString CourseTitle();
    static FName CourseMap(int32 Index);
    static float FinishX;
    void SetNightAmount(float Amount);
    static const TArray<float>& ObstaclePositions();
    static const TArray<float>& RampPositions();
    static float ObstacleHeight(int32 Index);
    static bool IsRideableObstacle(int32 Index);
    static float SurfaceHeightAt(float X);
    static float ObstacleHalfWidth(int32 Index);
    static float ObstacleTopAt(int32 Index, float X);
    static float RampJumpBonus(){return GetActiveCourse()==1?630.f:GetActiveCourse()==2?500.f:330.f;}

private:
    UPROPERTY() UProceduralMeshComponent* Terrain;
    UPROPERTY() UProceduralMeshComponent* ObstacleTops;
    UPROPERTY(Transient) TArray<UHierarchicalInstancedStaticMeshComponent*> Pools;
    UPROPERTY() UMaterialInstanceDynamic* LampMaterial;
    void Add(int32 Pool, FVector Position, FVector Scale, FRotator Rotation = FRotator::ZeroRotator);
    void Branch(int32 Pool, const FVector& A, const FVector& B, float Radius);
    void RimeTree(float X, float Y, float Scale, FRandomStream& Random);
    void Pine(float X, float Y, float Scale);
    void Lodge(float X, float Y);
    void Ground();
    void BuildChapterScenery();
    void BuildCourseFurniture();
    void BuildRideableSurfaces();
    void BuildVolcanicValley();
    static float ChapterHeight(float X);
};
