#include "SkiJourneyAtmosphere.h"
#include "SkiGameAudio.h"
#include "SkiJourneyProgress.h"
#include "SkiRunnerCharacter.h"
#include "SkiSampleTrail.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameViewportClient.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundWave.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

ASkiJourneyAtmosphere::ASkiJourneyAtmosphere()
{
    PrimaryActorTick.bCanEverTick=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    ConstructorHelpers::FObjectFinder<UStaticMesh> Ball(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    ConstructorHelpers::FObjectFinder<UMaterialInterface> SnowMat(TEXT("/Game/Ski25D/Materials/M_SnowFX.M_SnowFX"));
    Snow=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WindborneSnow"));
    Snow->SetupAttachment(RootComponent);Snow->SetStaticMesh(Ball.Object);Snow->SetMaterial(0,SnowMat.Object);
    Snow->SetCollisionEnabled(ECollisionEnabled::NoCollision);Snow->SetCastShadow(false);
    for(int32 I=0;I<3;++I)
    {
        auto* Pool=CreateDefaultSubobject<UInstancedStaticMeshComponent>(*FString::Printf(TEXT("Firework%d"),I));
        Pool->SetupAttachment(RootComponent);Pool->SetStaticMesh(Ball.Object);
        ConstructorHelpers::FObjectFinder<UMaterialInterface> Mat(*FString::Printf(TEXT("/Game/Ski25D/Materials/M_Firework%d.M_Firework%d"),I,I));
        Pool->SetMaterial(0,Mat.Object);Pool->SetCollisionEnabled(ECollisionEnabled::NoCollision);Pool->SetCastShadow(false);Sparks.Add(Pool);
    }
    for(int32 I=0;I<8;++I)
    {
        auto* Lamp=CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("TrailLight%d"),I));
        Lamp->SetupAttachment(RootComponent);Lamp->SetMobility(EComponentMobility::Movable);
        Lamp->SetIntensityUnits(ELightUnits::Lumens);Lamp->SetIntensity(0);Lamp->SetAttenuationRadius(1150);Lamp->SetCastShadows(false);
        Lamp->SetLightColor(FLinearColor(1,.57,.22));Lamps.Add(Lamp);
    }
    WindAudio=CreateDefaultSubobject<UAudioComponent>(TEXT("ColdWind"));WindAudio->SetupAttachment(RootComponent);
    MoonFill=CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("MoonFill"));MoonFill->SetupAttachment(RootComponent);
    MoonFill->SetMobility(EComponentMobility::Movable);MoonFill->SetCastShadows(false);
    MoonFill->SetRelativeRotation(FRotator(-48,-90,0));MoonFill->SetLightColor(FLinearColor(.32,.49,1));MoonFill->SetIntensity(0);
    WindAudio->bAutoActivate=false;WindAudio->bAllowSpatialization=false;
    ConstructorHelpers::FObjectFinder<USoundWave> Wind(TEXT("/Game/Ski25D/Audio/WinterWind.WinterWind"));
    if(Wind.Succeeded())WindAudio->SetSound(Wind.Object);
}

void ASkiJourneyAtmosphere::BeginPlay()
{
    Super::BeginPlay();
    for(TActorIterator<ADirectionalLight> It(GetWorld());It;++It){Sun=*It;break;}
    for(TActorIterator<ASkyLight> It(GetWorld());It;++It){Sky=*It;break;}
    for(TActorIterator<AExponentialHeightFog> It(GetWorld());It;++It){Fog=*It;break;}
    for(TActorIterator<ASkiSampleTrail> It(GetWorld());It;++It){Trail=*It;break;}
    const FTransform Hidden(FRotator::ZeroRotator,FVector::ZeroVector,FVector::ZeroVector);
    for(int32 I=0;I<420;++I)Snow->AddInstance(Hidden);
    for(auto* Pool:Sparks)for(int32 I=0;I<6*128;++I)Pool->AddInstance(Hidden);
    WindAudio->SetVolumeMultiplier(.1f);WindAudio->Play();
}

void ASkiJourneyAtmosphere::Tick(float Dt)
{
    Super::Tick(Dt);
    if(!Runner)for(TActorIterator<ASkiRunnerCharacter> It(GetWorld());It;++It){Runner=*It;break;}
    if(!Runner)return;
    const float RunTime=Runner->GetRunSeconds();
    if(RunTime<LastRunTime)
    {
        FXTime=0;NextBurst=0;BurstCount=0;bDayShot=bDuskShot=bNightShot=bReportWritten=false;
        for(auto& Burst:Bursts)Burst.Start=-100;
    }
    LastRunTime=RunTime;FXTime+=Dt;
    const int32 Chapter=ASkiSampleTrail::GetActiveCourse();
    const float SkyTime=RunTime*(Chapter==1?.55f:Chapter==2?.8f:1.f);
    const float Dusk=FMath::SmoothStep(14.f,32.f,SkyTime);
    const float Night=FMath::SmoothStep(30.f,47.f,SkyTime);
    const float LampAmount=FMath::SmoothStep(24.f,42.f,SkyTime);
    const float Gust=.35f+.65f*FMath::Square(FMath::Sin(RunTime*.19f));
    const FVector P=Runner->GetActorLocation();
    if(Sun)
    {
        auto* Light=Cast<UDirectionalLightComponent>(Sun->GetLightComponent());
        // Lift the sun through sunset, then retain a cool moonlike fill so the
        // snow silhouettes stay playable rather than becoming a black screen.
        Sun->SetActorRotation(FRotator(FMath::Lerp(-24.f,-8.f,Dusk),-38+RunTime*.2f,0));
        Light->SetIntensity(FMath::Lerp(FMath::Lerp(6.f,2.2f,Dusk),.75f,Night));
        const FLinearColor Day=Chapter==1?FLinearColor(.86,.95,1):Chapter==2?FLinearColor(.68,.82,1):FLinearColor(1,.88,.73);
        Light->SetLightColor(FMath::Lerp(FMath::Lerp(Day,FLinearColor(1,.37,.13),Dusk),FLinearColor(.35,.49,.9),Night));
    }
    MoonFill->SetIntensity(Night*1.2f);
    if(Sky)Sky->GetLightComponent()->SetIntensity(FMath::Lerp(2.f,.65f,Night));
    if(Fog)
    {
        Fog->SetActorLocation(FVector(P.X,0,P.Z-1100));
        auto* Component=Fog->GetComponent();
        Component->SetFogDensity(Chapter==1?.001f+Gust*.001f:Chapter==2?.004f+Gust*.003f:.0025f+Gust*.003f);
        Component->SetFogInscatteringColor(FMath::Lerp(FMath::Lerp(FLinearColor(.48,.66,.76),FLinearColor(.48,.23,.20),Dusk),FLinearColor(.018,.035,.09),Night));
        Component->SetFogMaxOpacity(.65f);
    }
    if(Trail)Trail->SetNightAmount(LampAmount);
    const int32 Nearest=FMath::FloorToInt((P.X-2700)/1800);
    for(int32 I=0;I<Lamps.Num();++I)
    {
        const float X=2700+(Nearest+I-2)*1800;
        Lamps[I]->SetWorldLocation(FVector(X+100,-200,ASkiSampleTrail::HeightAt(X)+395));
        Lamps[I]->SetIntensity(X>=2700&&X<ASkiSampleTrail::FinishX?LampAmount*1450:0);
    }
    const auto* Progress=Cast<USkiJourneyProgress>(GetGameInstance());
    WindAudio->SetVolumeMultiplier(Progress&&Progress->bAudioMuted?0:Runner->HasStarted()&&!Runner->HasFinished()?.06f+Gust*.12f:.025f);
    auto Wrap=[](float Value,float Range){return FMath::Fmod(FMath::Fmod(Value,Range)+Range,Range);};
    for(int32 I=0;I<420;++I)
    {
        const float X=P.X-1400+Wrap(I*947.31f-FXTime*430-P.X,5600);
        const float Z=P.Z-1700+Wrap(I*357.73f-FXTime*(190+I%7*18)-P.Z,3800);
        const float Y=-700+Wrap(I*731.19f+FMath::Sin(FXTime+I)*80,2300);
        const float Size=(.026f+(I%5)*.008f)*(I<150+Gust*270?1:0);
        Snow->UpdateInstanceTransform(I,FTransform(FRotator(-32,0,0),FVector(X,Y,Z),FVector(Size,Size,Size*(1.2f+Gust))),true,false,true);
    }
    Snow->MarkRenderStateDirty();
    const bool Celebrating=Runner->HasFinished()&&Runner->GetHearts()>0;
    const bool FireworksEnabled=Celebrating||(Night>.65f&&Runner->HasStarted()&&!Runner->HasFinished());
    if(Celebrating!=bWasCelebrating)
    {
        for(auto& Burst:Bursts)Burst.Start=-100;
        NextBurst=FXTime;BurstCount=0;CelebrationBursts=0;bWasCelebrating=Celebrating;
    }
    if(FireworksEnabled&&FXTime>=NextBurst)
    {
        const int32 Index=BurstCount%6;
        Bursts[Index]={FVector(P.X+1600+(BurstCount%3)*450,-2200-(BurstCount%2)*500,P.Z+900+(BurstCount%3)*140),FXTime,BurstCount%3};
        NextBurst=FXTime+(Celebrating?.72f:2.2f);++BurstCount;
        if(Celebrating)++CelebrationBursts;
        UE_LOG(LogTemp,Display,TEXT("SKI_FIREWORK burst=%d time=%.1f"),BurstCount,RunTime);
    }
    auto* Camera=UGameplayStatics::GetPlayerCameraManager(this,0);
    int32 Width=1280,Height=720;
    if(auto* PC=GetWorld()->GetFirstPlayerController())PC->GetViewportSize(Width,Height);
    const FRotator View=Camera?Camera->GetCameraRotation():FRotator(0,-90,0);
    const FVector Right=FRotationMatrix(View).GetUnitAxis(EAxis::Y),Up=FRotationMatrix(View).GetUnitAxis(EAxis::Z);
    const float HalfWidth=2600*FMath::Tan(FMath::DegreesToRadians(Camera?Camera->GetFOVAngle()*.5f:24.f));
    const float HalfHeight=HalfWidth*float(FMath::Max(1,Height))/FMath::Max(1,Width);
    VisibleSparks=0;
    for(int32 B=0;B<6;++B)
    {
        const FBurst& Burst=Bursts[B];const float Age=FXTime-Burst.Start;
        for(int32 C=0;C<3;++C)for(int32 I=0;I<128;++I)
        {
            FVector Position=Burst.Origin,Scale=FVector::ZeroVector;
            FRotator SparkRotation=FRotator::ZeroRotator;
            if(C==Burst.Color&&I==0&&Age>=.7f&&Age-Dt<.7f&&FireworksEnabled)ASkiGameAudio::Event(this,TEXT("Firework"),Celebrating?.48f:.22f);
            // Far-sky effects share the camera's vertical parallax, staying in
            // view while the skier drops rapidly down the mountain.
            Position.Z=P.Z+300+B*130;
            if(Celebrating&&Camera)
            {
                const float Horizontal[]={-.66f,.65f,-.18f,.48f,-.52f,.12f};
                Position=Camera->GetCameraLocation()+View.Vector()*2600+Right*(HalfWidth*Horizontal[B])+Up*(HalfHeight*(B%3==2?.72f:.38f));
            }
            if(C==Burst.Color&&Age>=0&&Age<(Celebrating?4.2f:3.1f)&&FireworksEnabled)
            {
                if(Age<.7f&&I<12)
                {
                    Position-=Up*((Celebrating?HalfHeight*.8f:650)*(1-Age/.7f)+I*15);Scale=FVector((Celebrating?.23f:.14f)*(1-I/14.f));
                }
                else if(Age>=.7f)
                {
                    const float T=Age-.7f,Angle=Celebrating?(I/2)*(2*PI/64.f):(I/2)*2.399963f;
                    const float Radius=Celebrating?HalfHeight*.72f*(.86f+.14f*FMath::Sin((I/2)*13.7f+B)):FMath::Sqrt(((I/2)+.5f)/64.f)*340;
                    const float Spread=Celebrating?(1-FMath::Exp(-T*1.8f))*(I%2?.91f:1.f):T;
                    Position+=Right*(FMath::Cos(Angle)*Radius*Spread)+Up*(FMath::Sin(Angle)*Radius*Spread-(Celebrating?HalfHeight*.045f:95)*T*T);
                    const float Fade=FMath::Pow(FMath::Clamp(1-T/(Celebrating?3.5f:2.4f),0.f,1.f),.7f);
                    const float Size=(Celebrating?.12f:.15f)*Fade*(I%2?.6f:1.f);
                    const FVector Radial=Right*FMath::Cos(Angle)+Up*(FMath::Sin(Angle)-T*.12f);
                    SparkRotation=FRotationMatrix::MakeFromZ(Radial).Rotator();
                    Scale=FVector(Size,Size,Celebrating?(.36f+.28f*FMath::Sin(FMath::Min(T,1.f)*PI*.5f))*Fade:Size*1.8f);
                }
                if(!Scale.IsNearlyZero())++VisibleSparks;
            }
            Sparks[C]->UpdateInstanceTransform(B*128+I,FTransform(SparkRotation,Position,Scale),true,false,true);
        }
    }
    for(auto* Pool:Sparks)Pool->MarkRenderStateDirty();
    if(FParse::Param(FCommandLine::Get(),TEXT("SkiSmokeTest"))&&!FParse::Param(FCommandLine::Get(),TEXT("SkiNoInputTest")))
    {
        auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("SkiQA/%s.png"),Name),true,false);};
        if(!bDayShot&&RunTime>8){Shot(TEXT("Daylight"));bDayShot=true;}
        if(!bDuskShot&&Dusk>.8f){Shot(TEXT("Sunset"));bDuskShot=true;}
        if(!bNightShot&&Night>.98f&&BurstCount>2&&FXTime-Bursts[(BurstCount-1)%6].Start>1.35f){Shot(TEXT("NightFireworks"));bNightShot=true;}
        if(Runner->HasFinished()&&!bReportWritten)
        {
            bReportWritten=true;
            FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"night\":%.3f,\"lamps\":%.3f,\"fireworks\":%d,\"route_metres\":%.0f}"),Night>.98f&&LampAmount>.98f&&BurstCount>=3&&bNightShot?TEXT("true"):TEXT("false"),Night,LampAmount,BurstCount,(ASkiSampleTrail::FinishX-200)*.01f),*(FPaths::ProjectSavedDir()/TEXT("SkiQA/environment.json")));
        }
    }
}
