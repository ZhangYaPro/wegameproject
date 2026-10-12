#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "SkiSampleGameMode.generated.h"

UCLASS()
class SKI25D_API ASkiSampleHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    virtual void NotifyHitBoxClick(FName BoxName) override;
    float SceneWeight(int32 Index)const{return MenuWeights[Index];}
private:
    void DrawSceneImage(int32 Index,float X,float Y,float W,float H,float Alpha,bool Moving);
    float MenuWeights[3]={0,0,0};
    void DrawMenu(class ASkiRunnerCharacter* P,float W);
    void Button(int32 Action,const FString& Label,const FString& Detail,float X,float Y,float W,float H,bool Active=false);
    void Line(float X,float Y,float X2,float Y2,FLinearColor Color,float Thickness=1);
    UPROPERTY() TArray<class UTexture2D*> MenuImages;
    void Text(const FString& S,float X,float Y,float Size,FLinearColor C);
    void Panel(float X,float Y,float W,float H,FLinearColor C);
};

UCLASS()
class SKI25D_API ASkiSampleGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ASkiSampleGameMode();
    virtual void InitGame(const FString& MapName,const FString& Options,FString& ErrorMessage) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
private:
    bool RunExperienceChecks(float Dt);
    bool RunFinaleChecks(float Dt);
    float QASeconds=0;
    float QAEndSeconds=0;
    int32 PreviewStep=-1;
    bool bPreviewCaptured=false;
};
