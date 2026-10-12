#include "SkiRunnerCharacter.h"
#include "SkiGameAudio.h"
#include "Components/PointLightComponent.h"
#include "SkiSampleTrail.h"
#include "SkiJourneyProgress.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "UObject/ConstructorHelpers.h"

ASkiRunnerCharacter::ASkiRunnerCharacter()
{
    PrimaryActorTick.bCanEverTick=true;
    GetCapsuleComponent()->InitCapsuleSize(28,88);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Ignore);
    bUseControllerRotationPitch=bUseControllerRotationYaw=bUseControllerRotationRoll=false;
    auto* M=GetCharacterMovement();
    M->bOrientRotationToMovement=false; M->MaxWalkSpeed=1060; M->MaxAcceleration=6000;
    M->JumpZVelocity=900; M->GravityScale=1.6; M->AirControl=1;
    M->GroundFriction=.8; M->BrakingDecelerationWalking=1800;M->MaxStepHeight=36;
    M->bMaintainHorizontalGroundVelocity=false;
    M->SetWalkableFloorAngle(67);M->SetPlaneConstraintNormal(FVector(0,1,0));
    M->bUseFlatBaseForFloorChecks=true;
    M->SetPlaneConstraintEnabled(true); M->bSnapToPlaneAtStart=true;
    CameraBoom=CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);CameraBoom->SetUsingAbsoluteRotation(true);
    CameraBoom->SetRelativeRotation(FRotator(-10,-90,0));CameraBoom->TargetArmLength=1900;
    CameraBoom->TargetOffset=FVector(480,0,130);CameraBoom->bUsePawnControlRotation=false;
    CameraBoom->bDoCollisionTest=false;CameraBoom->bEnableCameraLag=true;CameraBoom->CameraLagSpeed=7;
    CameraBoom->bUseCameraLagSubstepping=true;CameraBoom->CameraLagMaxDistance=160;
    FollowCamera=CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom);FollowCamera->FieldOfView=60;
    TrickPivot=CreateDefaultSubobject<USceneComponent>(TEXT("TrickPivot"));
    TrickPivot->SetupAttachment(RootComponent);TrickPivot->SetRelativeLocation(FVector(0,0,-6));
    SkierVisual=CreateDefaultSubobject<USceneComponent>(TEXT("SkierVisual"));
    SkierVisual->SetupAttachment(TrickPivot);SkierVisual->SetRelativeLocation(FVector(0,0,-80));
    ConstructorHelpers::FObjectFinder<UMaterialInterface> O(TEXT("/Game/Ski25D/Materials/M_RaceRed.M_RaceRed"));
    ConstructorHelpers::FObjectFinder<UMaterialInterface> N(TEXT("/Game/Ski25D/Materials/M_RaceNavy.M_RaceNavy"));
    ConstructorHelpers::FObjectFinder<UMaterialInterface> W(TEXT("/Game/Ski25D/Materials/M_Snow.M_Snow"));
    ConstructorHelpers::FObjectFinder<UMaterialInterface> G(TEXT("/Game/Ski25D/Materials/M_RaceVisor.M_RaceVisor"));
    ConstructorHelpers::FObjectFinder<UStaticMesh> Disc(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    ConstructorHelpers::FObjectFinder<UMaterialInterface> Ice(TEXT("/Game/Ski25D/Materials/M_Snow.M_Snow"));
    ConstructorHelpers::FObjectFinder<UMaterialInterface> PickupGold(TEXT("/Game/Ski25D/Materials/M_WarmGlass.M_WarmGlass"));
    Medals=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WinterMedals"));
    Crystals=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RimeCrystals"));
    for(auto* Pool : {Medals,Crystals})
    {
        Pool->SetupAttachment(RootComponent);Pool->SetAbsolute(true,true,true);
        Pool->SetCollisionEnabled(ECollisionEnabled::NoCollision);Pool->SetCastShadow(false);
    }
    Medals->SetStaticMesh(Disc.Object);Medals->SetMaterial(0,PickupGold.Object);
    Crystals->SetStaticMesh(Cube.Object);Crystals->SetMaterial(0,Ice.Object);
    ConstructorHelpers::FObjectFinder<UStaticMesh> SnowBall(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    ConstructorHelpers::FObjectFinder<UMaterialInterface> SprayMat(TEXT("/Game/Ski25D/Materials/M_SnowFX.M_SnowFX"));
    SkiSpray=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SkiTailSnow"));
    SkiSpray->SetupAttachment(RootComponent);SkiSpray->SetAbsolute(true,true,true);
    SkiSpray->SetStaticMesh(SnowBall.Object);SkiSpray->SetMaterial(0,SprayMat.Object);
    SkiSpray->SetCollisionEnabled(ECollisionEnabled::NoCollision);SkiSpray->SetCastShadow(false);
    VictoryLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("VictoryPortraitLight"));
    VictoryLight->SetupAttachment(RootComponent);VictoryLight->SetRelativeLocation(FVector(90,240,155));
    VictoryLight->SetIntensityUnits(ELightUnits::Lumens);VictoryLight->SetIntensity(0);VictoryLight->SetAttenuationRadius(950);VictoryLight->SetCastShadows(false);VictoryLight->SetLightColor(FLinearColor(1,.80,.65));
    Shape(TEXT("Jacket"),TEXT("/Game/Ski25D/Skier/SM_Jacket"),FVector(0,0,108),FVector(.30,.47,.58),O.Object);
    Shape(TEXT("Hood"),TEXT("/Game/Ski25D/Skier/SM_Collar"),FVector(0,0,137),FVector(.26,.30,.16),N.Object);
    ConstructorHelpers::FObjectFinder<UMaterialInterface> Skin(TEXT("/Game/Ski25D/Materials/M_RaceSkin.M_RaceSkin"));
    Shape(TEXT("Face"),TEXT("/Game/Ski25D/Skier/SM_Helmet"),FVector(11,0,151),FVector(.29,.27,.31),Skin.Object);
    Shape(TEXT("Helmet"),TEXT("/Game/Ski25D/Skier/SM_Helmet"),FVector(7,0,167),FVector(.36,.36,.24),W.Object);
    Shape(TEXT("Goggles"),TEXT("/Game/Ski25D/Skier/SM_Goggles"),FVector(26,0,156),FVector(.065,.31,.11),G.Object);
    Shape(TEXT("Chin"),TEXT("/Game/Ski25D/Skier/SM_Collar"),FVector(10,0,144),FVector(.23,.26,.18),N.Object);
    Shape(TEXT("ChestBand"),TEXT("Cube"),FVector(13,0,124),FVector(.035,.39,.065),W.Object);
    Shape(TEXT("Zip"),TEXT("Cube"),FVector(14,0,108),FVector(.025,.025,.40),N.Object);
    for(int Sign : {-1,1})
    {
        const float Y=Sign*13.f;
        Shape(*FString::Printf(TEXT("Thigh%d"),Sign),TEXT("/Game/Ski25D/Skier/SM_Trousers"),FVector(-7,Y,65),FVector(.22,.24,.44),N.Object,FRotator(-25,0,0));
        Shape(*FString::Printf(TEXT("Shin%d"),Sign),TEXT("/Game/Ski25D/Skier/SM_Shin"),FVector(1,Y,34),FVector(.19,.21,.37),N.Object,FRotator(22,0,0));
        auto* Boot=Shape(*FString::Printf(TEXT("Boot%d"),Sign),TEXT("/Game/Ski25D/Skier/SM_Boot"),FVector(7,Y,14),FVector(.37,.20,.25),W.Object);
        auto* Ski=Shape(*FString::Printf(TEXT("Ski%d"),Sign),TEXT("/Game/Ski25D/Skier/SM_Ski"),FVector(0,Y,3),FVector(.90,1,1),O.Object);
        auto* Stripe=Shape(*FString::Printf(TEXT("SkiStripe%d"),Sign),TEXT("Cube"),FVector::ZeroVector,FVector(1.4,.045,.012),W.Object);
        Stripe->SetupAttachment(Ski);Stripe->SetRelativeLocation(FVector(0,0,1));Stripe->SetRelativeScale3D(FVector(1.4,.045,.012));
        auto* Buckle=Shape(*FString::Printf(TEXT("BootBuckle%d"),Sign),TEXT("Cube"),FVector::ZeroVector,FVector(.025,.9,.13),N.Object);
        Buckle->SetupAttachment(Boot);Buckle->SetRelativeLocation(FVector(38,0,3));Buckle->SetRelativeScale3D(FVector(.035,.87,.14));
        Shape(*FString::Printf(TEXT("Sleeve%d"),Sign),TEXT("/Game/Ski25D/Skier/SM_UpperArm"),FVector(4,Sign*26,113),FVector(.23,.22,.31),O.Object);
        Shape(*FString::Printf(TEXT("Forearm%d"),Sign),TEXT("/Game/Ski25D/Skier/SM_Forearm"),FVector(18,Sign*31,90),FVector(.20,.19,.29),W.Object);
        Shape(*FString::Printf(TEXT("Glove%d"),Sign),TEXT("/Game/Ski25D/Skier/SM_Glove"),FVector(32,Sign*31,81),FVector(.16,.14,.18),N.Object);
        Shape(*FString::Printf(TEXT("Pole%d"),Sign),TEXT("Cylinder"),FVector(-5,Sign*34,47),FVector(.018,.018,1.02),N.Object,FRotator(-30,0,0));
    }
    ConstructorHelpers::FObjectFinder<UMaterialInterface> FlagRed(TEXT("/Game/Ski25D/Materials/M_FlagRed.M_FlagRed"));
    ConstructorHelpers::FObjectFinder<UMaterialInterface> FlagGold(TEXT("/Game/Ski25D/Materials/M_FlagGold.M_FlagGold"));
    Shape(TEXT("RightArmBorder"),TEXT("/Game/Ski25D/Skier/SM_FlagCloth"),FVector(0,35.9,118),FVector(.165),W.Object);
    Shape(TEXT("RightArmFlag"),TEXT("/Game/Ski25D/Skier/SM_FlagCloth"),FVector(0,36,118),FVector(.15),FlagRed.Object);
    Shape(TEXT("RightArmStars"),TEXT("/Game/Ski25D/Skier/SM_FlagStars"),FVector(0,36.2,118),FVector(.15),FlagGold.Object);
    // Character faces +X: its left chest is -Y. A front-facing, correctly
    // oriented 3:2 badge sits below the white stripe, clear of the central zip.
    Shape(TEXT("LeftChestBorder"),TEXT("/Game/Ski25D/Skier/SM_FlagCloth"),FVector(15.5,-10,115),FVector(.115),W.Object,FRotator(0,-90,0));
    Shape(TEXT("LeftChestFlag"),TEXT("/Game/Ski25D/Skier/SM_FlagCloth"),FVector(15.65,-10,115),FVector(.105),FlagRed.Object,FRotator(0,-90,0));
    Shape(TEXT("LeftChestStars"),TEXT("/Game/Ski25D/Skier/SM_FlagStars"),FVector(15.8,-10,115),FVector(.105),FlagGold.Object,FRotator(0,-90,0));

}

UStaticMeshComponent* ASkiRunnerCharacter::Shape(const TCHAR* Name,const TCHAR* MeshName,FVector P,FVector S,UMaterialInterface* Mat,FRotator R)
{
    auto* C=CreateDefaultSubobject<UStaticMeshComponent>(Name);C->SetupAttachment(SkierVisual);
    const FString AssetPath=FString(MeshName).StartsWith(TEXT("/"))?FString(MeshName):FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"),MeshName,MeshName);
    ConstructorHelpers::FObjectFinder<UStaticMesh> ShapeAsset(*AssetPath);
    C->SetStaticMesh(ShapeAsset.Object);if(Mat)C->SetMaterial(0,Mat);
    C->SetRelativeLocation(P);C->SetRelativeScale3D(S);C->SetRelativeRotation(R);
    C->SetCollisionEnabled(ECollisionEnabled::NoCollision);return C;
}
void ASkiRunnerCharacter::BeginPlay()
{
    Super::BeginPlay();GetMesh()->SetHiddenInGame(true,true);
    GetCharacterMovement()->SetPlaneConstraintOrigin(FVector::ZeroVector);
    bNoInputTest=FParse::Param(FCommandLine::Get(),TEXT("SkiNoInputTest"));
    bCrashTest=FParse::Param(FCommandLine::Get(),TEXT("SkiCrashTest"));
    bAutomation=FParse::Param(FCommandLine::Get(),TEXT("SkiSmokeTest"))||bNoInputTest;
    if(bAutomation)
    {
        IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir()/TEXT("SkiQA")),true);
        IFileManager::Get().Delete(*(FPaths::ProjectSavedDir()/(bNoInputTest?TEXT("SkiQA/no_input.json"):TEXT("SkiQA/smoke.json"))));
    }
    CacheBodyPose();
    bPresentationTest=FParse::Param(FCommandLine::Get(),TEXT("SkiPresentationTest"));
    if(bPresentationTest){bAutomation=true;bNoInputTest=true;IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir()/TEXT("SkiQA")),true);}
    RestartRun();
    if(auto* Progress=Cast<USkiJourneyProgress>(GetGameInstance()))if(Progress->bStartOnArrival){Progress->bStartOnArrival=false;bStarted=true;}
    if(FParse::Param(FCommandLine::Get(),TEXT("SkiLandingCheck"))){RunLandingChecks();return;}
    if(bAutomation&&!bPresentationTest){bStarted=true;NextScreenshot=1;}
}
void ASkiRunnerCharacter::SetupPlayerInputComponent(UInputComponent* I)
{
    Super::SetupPlayerInputComponent(I);
    I->BindKey(EKeys::SpaceBar,IE_Pressed,this,&ASkiRunnerCharacter::StartOrJump);
    auto& JumpRelease=I->BindKey(EKeys::SpaceBar,IE_Released,this,&ASkiRunnerCharacter::ReleaseJump);JumpRelease.bExecuteWhenPaused=true;
    I->BindKey(EKeys::Enter,IE_Pressed,this,&ASkiRunnerCharacter::StartOrJump);
    I->BindKey(EKeys::Left,IE_Pressed,this,&ASkiRunnerCharacter::MenuLeft);
    I->BindKey(EKeys::Right,IE_Pressed,this,&ASkiRunnerCharacter::MenuRight);
    I->BindKey(EKeys::Up,IE_Pressed,this,&ASkiRunnerCharacter::MenuUp);
    I->BindKey(EKeys::Down,IE_Pressed,this,&ASkiRunnerCharacter::MenuDown);
    I->BindKey(EKeys::One,IE_Pressed,this,&ASkiRunnerCharacter::SelectRime);
    I->BindKey(EKeys::Two,IE_Pressed,this,&ASkiRunnerCharacter::SelectBeidahu);
    I->BindKey(EKeys::Three,IE_Pressed,this,&ASkiRunnerCharacter::SelectChangbai);
    auto& EnterRelease=I->BindKey(EKeys::Enter,IE_Released,this,&ASkiRunnerCharacter::ReleaseJump);EnterRelease.bExecuteWhenPaused=true;
    I->BindKey(EKeys::S,IE_Pressed,this,&ASkiRunnerCharacter::BrakePressed);
    auto& BrakeRelease=I->BindKey(EKeys::S,IE_Released,this,&ASkiRunnerCharacter::BrakeReleased);BrakeRelease.bExecuteWhenPaused=true;
    auto& Mute=I->BindKey(EKeys::M,IE_Pressed,this,&ASkiRunnerCharacter::ToggleAudio);Mute.bExecuteWhenPaused=true;
    auto& Pause=I->BindKey(EKeys::Escape,IE_Pressed,this,&ASkiRunnerCharacter::TogglePause);Pause.bExecuteWhenPaused=true;
    auto& Restart=I->BindKey(EKeys::R,IE_Pressed,this,&ASkiRunnerCharacter::RestartRun);Restart.bExecuteWhenPaused=true;
}
void ASkiRunnerCharacter::RestartRun()
{
    UGameplayStatics::SetGamePaused(this,false);bRunPaused=false;bStarted=false;bFinished=false;
    MenuPage=MenuFocus=HistoryPage=0;SelectedCourse=ASkiSampleTrail::GetActiveCourse();
    VictoryLight->SetIntensity(0);
    Hearts=MaxHearts;Score=Landings=0;Time=HurtUntil=DistanceScore=0;LastRegion=-1;
    PassedObstacles.Empty();UsedRamps.Empty();Message.Empty();MessageTime=0;
    ForwardSpeed=760;AirSeconds=LandingPulse=0;bBraking=false;AirBonus=RampLaunches=0;
    SpinVelocity=FallRemaining=SprayAccumulator=0;Crashes=NextSpray=SprayEmitted=0;
    AirHoldSeconds=TakeoffPitch=ReturnElapsed=ReturnDuration=ReturnStartAngle=ReturnTargetAngle=ReturnInitialSpeed=0;bReturningUpright=false;
    WobbleRemaining=WobbleAngle=PresentationTime=DeathBlend=0;
    ApplyBodyPose(0,0,0);SkierVisual->SetRelativeScale3D(FVector::OneVector);
    bCapturedFall=bCapturedSpray=false;
    FallCaptureStage=0;
    SkiSnow.SetNum(192);for(auto& Particle:SkiSnow)Particle=FSkiSnow();
    SkiSpray->ClearInstances();SkiSpray->SetWorldTransform(FTransform::Identity);SprayRandom.Initialize(7321);
    for(int32 I=0;I<SkiSnow.Num();++I)SkiSpray->AddInstance(FTransform(FRotator::ZeroRotator,FVector::ZeroVector,FVector::ZeroVector),true);
    TopSpeed=MaxAirHeight=MaxArm=MaxSlope=0;MinArm=10000;
    bBrakeVerified=false;
    bJumpHeld=bSpinning=bFlightDamaged=bWasAirborne=bReleasedThisFlight=bCapturedFlip=false;
    SpinAngle=BasePitch=VisualPitch=BoostUntil=RewardTime=0;
    PendingFlips=TotalFlips=BestCombo=Collected=GroundPickups=AirPickups=PickupBonus=ReleasedFlights=HoldFlights=0;
    RewardMessage.Empty();
    LastLandingTime=-100;LastLandingBonus=LastLandingFlips=0;LandingFlipHistory.Empty();
    TrickPivot->SetRelativeRotation(FRotator::ZeroRotator);
    TrickPivot->SetRelativeLocation(FVector(0,0,-6));
    CameraBoom->TargetArmLength=1900;CameraBoom->TargetOffset=FVector(440,0,130);FollowCamera->FieldOfView=56;
    CameraBoom->SetRelativeRotation(FRotator(-10,-90,0));FollowCamera->SetRelativeLocation(FVector::ZeroVector);
    FollowCamera->PostProcessBlendWeight=0;
    SetActorLocation(FVector(200,0,ASkiSampleTrail::HeightAt(200)+91),false);
    SetActorRotation(FRotator::ZeroRotator);GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);StopJumping();
    SkierVisual->SetVisibility(true,true);LastX=200;LastFeet=ASkiSampleTrail::HeightAt(200)+3;
    ResetPickups();PreviousPickupPosition=GetActorLocation();
    UE_LOG(LogTemp,Display,TEXT("SKI_RUN_RESET"));
}
void ASkiRunnerCharacter::StartOrJump()
{
    if(bRunPaused||IsCelebrating()||(!bFinished&&FallRemaining>0))return;
    if(!bStarted){MenuConfirm();return;}
    bJumpHeld=true;
    if(bFinished)
    {
        if(Hearts>0){UGameplayStatics::OpenLevel(this,ASkiSampleTrail::CourseMap((ASkiSampleTrail::GetActiveCourse()+1)%3));return;}
        RestartRun();bStarted=true;return;
    }
    if(!bStarted){bStarted=true;PresentationTime=0;ShowMessage(TEXT("轻按起跳，长按空翻；松手调整落地姿态"),4);return;}
    if(GetCharacterMovement()->IsMovingOnGround())
    {
        const float UphillMomentum=FMath::Clamp(FMath::Tan(FMath::DegreesToRadians(GetSlopeDegrees()))*ForwardSpeed,0.f,350.f);
        GetCharacterMovement()->JumpZVelocity=900+ForwardSpeed*.13f+UphillMomentum;
        for(float X:ASkiSampleTrail::RampPositions())
            if(X-GetActorLocation().X>-80&&X-GetActorLocation().X<450)
                GetCharacterMovement()->JumpZVelocity+=ASkiSampleTrail::RampJumpBonus();
        StopJumping();Jump();ASkiGameAudio::Event(this,TEXT("Jump"));
    }
}
void ASkiRunnerCharacter::BrakePressed(){bBraking=true;}
void ASkiRunnerCharacter::SelectCourse(int32 Index){if(!bStarted){SelectedCourse=Index;MenuPage=1;}else if(bFinished&&!IsCelebrating())UGameplayStatics::OpenLevel(this,ASkiSampleTrail::CourseMap(Index));}
void ASkiRunnerCharacter::SelectRime(){SelectCourse(0);}
void ASkiRunnerCharacter::SelectBeidahu(){SelectCourse(1);}
void ASkiRunnerCharacter::SelectChangbai(){SelectCourse(2);}
void ASkiRunnerCharacter::PreviewCoursePoint(float Fraction)
{
    bStarted=true;const float X=FMath::Lerp(200.f,ASkiSampleTrail::FinishX-3500,Fraction);
    SetActorLocation(FVector(X,0,ASkiSampleTrail::HeightAt(X)+91));GetCharacterMovement()->StopMovementImmediately();
    LastX=X;LastFeet=ASkiSampleTrail::HeightAt(X);Time=Fraction*100;
}
void ASkiRunnerCharacter::BrakeReleased(){bBraking=false;}
float ASkiRunnerCharacter::CruiseSpeedForSlope(float Slope,bool Braking,bool Boosted)
{
    if(Braking)return 380;
    const float Incline=FMath::Sin(FMath::DegreesToRadians(Slope));
    return FMath::Clamp(1060+FMath::Max(0.f,-Incline)*250-FMath::Max(0.f,Incline)*160+(Boosted?70:0),760.f,1410.f);
}
void ASkiRunnerCharacter::TogglePause()
{
    if(!bStarted){MenuPage=MenuFocus=0;return;}
    if(bFinished){if(!IsCelebrating())RestartRun();return;}
    bRunPaused=!bRunPaused;
    if(!bRunPaused)if(auto* PC=Cast<APlayerController>(GetController()))
    {
        bBraking=PC->IsInputKeyDown(EKeys::S);
        bJumpHeld=PC->IsInputKeyDown(EKeys::SpaceBar)||PC->IsInputKeyDown(EKeys::Enter);
    }
    UGameplayStatics::SetGamePaused(this,bRunPaused);
}
void ASkiRunnerCharacter::ShowMessage(const FString& S,float Seconds){Message=S;MessageTime=Seconds;}
float ASkiRunnerCharacter::GetProgress()const{return bFinished&&Hearts>0?1.f:FMath::Clamp((GetActorLocation().X-200)/(ASkiSampleTrail::FinishX-200),0.f,1.f);}
FString ASkiRunnerCharacter::GetRegion()const
{
    if(ASkiSampleTrail::GetActiveCourse()==1)return GetProgress()<.3f?TEXT("北大湖 · 缆车雪场"):GetProgress()<.75f?TEXT("北大湖 · 分级飞坡"):TEXT("北大湖 · 青春竞速");
    if(ASkiSampleTrail::GetActiveCourse()==2)return GetProgress()<.3f?TEXT("长白岩谷 · 玄武岩雪径"):GetProgress()<.75f?TEXT("蓝冰峡谷 · 冰瀑飞驰"):TEXT("火山雪境 · 灯火归途");
    const float X=GetActorLocation().X;
    if(X<2500)return TEXT("长春出发 · 高处启程");if(X<17000)return TEXT("吉林雾凇 · 雪谷俯冲");
    if(X<42000)return TEXT("白山松水 · 连续飞坡");
    if(X<53000)return TEXT("长白山麓 · 林海飞驰");
    if(X<65500)return TEXT("查干冬捕 · 渔猎冬韵");
    if(X<79000)return TEXT("延边风情 · 暖雪人家");
    if(X<93000)return TEXT("长春记忆 · 雪夜电车");
    return TEXT("青春长春 · 灯火迎宾");
}
float ASkiRunnerCharacter::GetDropMetres()const{return FMath::Max(0.f,(ASkiSampleTrail::HeightAt(200)-ASkiSampleTrail::HeightAt(GetActorLocation().X))*.01f);}
float ASkiRunnerCharacter::GetSlopeDegrees()const
{
    const float X=GetActorLocation().X;
    if(GetCharacterMovement()->IsMovingOnGround()&&GetCharacterMovement()->CurrentFloor.IsWalkableFloor())
    {
        const FVector N=GetCharacterMovement()->CurrentFloor.HitResult.ImpactNormal;
        return FMath::RadiansToDegrees(FMath::Atan2(-N.X,N.Z));
    }
    return FMath::RadiansToDegrees(FMath::Atan2(ASkiSampleTrail::HeightAt(X+30)-ASkiSampleTrail::HeightAt(X-30),60));
}
bool ASkiRunnerCharacter::IsAirborne()const{return GetCharacterMovement()->IsFalling();}
void ASkiRunnerCharacter::Hurt()
{
    if(Time<HurtUntil||bFinished)return;--Hearts;HurtUntil=Time+1.9f;
    // Landed runs inside movement, before the swept obstacle check. Revoke
    // rewards if that same touchdown hits an obstacle on the following tick.
    if(Time-LastLandingTime<.12f&&LastLandingBonus>0)
    {
        AirBonus-=LastLandingBonus;TotalFlips-=LastLandingFlips;--Landings;
        if(LastLandingFlips>0){if(bLastLandingReleased)--ReleasedFlights;else --HoldFlights;}
        if(!LandingFlipHistory.IsEmpty())LandingFlipHistory.Pop();
        BestCombo=0;for(int32 Flips:LandingFlipHistory)BestCombo=FMath::Max(BestCombo,Flips);
        LastLandingBonus=LastLandingFlips=0;RewardTime=0;RefreshScore();
    }
    bFlightDamaged=true;PendingFlips=0;BoostUntil=0;
    BeginFallAnimation();ASkiGameAudio::Event(this,TEXT("Crash"));
    ForwardSpeed=FMath::Max(420.f,ForwardSpeed*.55f);
    ShowMessage(TEXT("撞击减速！提前起跳，或按住 S 调整节奏"));
    UE_LOG(LogTemp,Display,TEXT("SKI_HIT hearts=%d x=%.0f"),Hearts,GetActorLocation().X);
    if(Hearts<=0)FinishRun();
}
void ASkiRunnerCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);if(!bStarted||bFinished)return;
    const float LandingSlope=FMath::RadiansToDegrees(FMath::Atan2(-Hit.ImpactNormal.X,Hit.ImpactNormal.Z));
    const float Error=FMath::Abs(FMath::FindDeltaAngleDegrees(VisualPitch,LandingSlope));
    if(AirSeconds>=.18f)
    {
        LandingPulse=.24f;
        if(Error<=100&&!bFlightDamaged)
        {
            const bool Clean=Error<=65;
            if(!Clean)
            {
                WobbleRemaining=.65f;WobbleAngle=FMath::Clamp(FMath::FindDeltaAngleDegrees(LandingSlope,VisualPitch),-32.f,32.f);
                PendingFlips=0;
                ShowMessage(TEXT("稳住了，继续前进！"),1.5f);
            }
            ASkiGameAudio::Event(this,TEXT("Land"),Clean?.65f:.85f);
            ++Landings;
            const int32 Bonus=FMath::RoundToInt(AirSeconds*(Clean?60:25))+PendingFlips*150+FMath::Max(0,PendingFlips-1)*100;
            AirBonus+=Bonus;TotalFlips+=PendingFlips;BestCombo=FMath::Max(BestCombo,PendingFlips);
            LastLandingBonus=Bonus;LastLandingFlips=PendingFlips;LastLandingTime=Time;
            bLastLandingReleased=bReleasedThisFlight;LandingFlipHistory.Add(PendingFlips);
            if(PendingFlips>0)
            {
                if(bReleasedThisFlight)++ReleasedFlights;else ++HoldFlights;
                ASkiGameAudio::Event(this,TEXT("Trick"));
                BoostUntil=Time+1.5f;
                RewardMessage=FString::Printf(TEXT("后空翻 ×%d  稳稳落地 +%d"),PendingFlips,Bonus);RewardTime=2;
            }
            else if(Clean&&Time>=HurtUntil)ShowMessage(FString::Printf(TEXT("顺坡落地 +%d"),Bonus),1);
            UE_LOG(LogTemp,Display,TEXT("SKI_TRICK_LAND flips=%d error=%.1f bonus=%d released=%d"),PendingFlips,Error,Bonus,bReleasedThisFlight);
        }
        else if(!bFlightDamaged)
        {
            if(Time>=HurtUntil)
            {
                Hurt();bJumpHeld=false;
                RewardTime=0;RewardMessage.Empty();
                ShowMessage(TEXT("调整姿态，再出发！"),2.5);
                UE_LOG(LogTemp,Display,TEXT("SKI_CRASH error=%.1f hearts=%d crashes=%d"),Error,Hearts,Crashes);
            }
        }
    }
    PendingFlips=0;SpinAngle=SpinVelocity=0;bSpinning=false;bWasAirborne=false;bFlightDamaged=false;bReleasedThisFlight=false;
    AirHoldSeconds=0;bReturningUpright=false;
    BasePitch=VisualPitch=LandingSlope;AirSeconds=0;
    if(bAutomation)ReleaseJump();
    RefreshScore();
}
void ASkiRunnerCharacter::UpdateCamera(float Dt)
{
    if(UpdatePresentationCamera(Dt))return;
    CameraBoom->SetRelativeRotation(FMath::RInterpTo(CameraBoom->GetRelativeRotation(),FRotator(-10,-90,0),Dt,2.5f));
    const FVector P=GetActorLocation();
    const float SpeedAlpha=FMath::Clamp((ForwardSpeed-760)/650,0.f,1.f);
    const float AirHeight=FMath::Max(0.f,P.Z-88-ASkiSampleTrail::SurfaceHeightAt(P.X));
    FVector Landing;const float LandingTime=PredictLanding(Landing);
    const float Lead=520+SpeedAlpha*440;
    const float DropAhead=ASkiSampleTrail::HeightAt(P.X+Lead)-ASkiSampleTrail::HeightAt(P.X);
    const float LandingDrop=IsAirborne()?FMath::Clamp(P.Z-Landing.Z-88,0.f,2400.f):0;
    const float TargetArm=FMath::Clamp(1850+SpeedAlpha*650+AirHeight*.48f+LandingDrop*.18f+FMath::Abs(DropAhead)*.18f,1850.f,3650.f);
    const float LookDown=FMath::Clamp(DropAhead*.35f-AirHeight*.18f-(IsAirborne()&&LandingTime<.8f?LandingDrop*.12f:0),-680.f,160.f);
    const FVector Target(Lead,0,140+LookDown);
    CameraBoom->TargetArmLength=FMath::FInterpTo(CameraBoom->TargetArmLength,TargetArm,Dt,TargetArm>CameraBoom->TargetArmLength?3.8f:1.8f);
    CameraBoom->TargetOffset=FMath::VInterpTo(CameraBoom->TargetOffset,Target,Dt,3);
    // A fixed lens keeps the scene's perspective stable; the boom supplies zoom.
    FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView,60.f,Dt,3));
    LandingPulse=FMath::Max(0.f,LandingPulse-Dt);
    FollowCamera->SetRelativeLocation(FVector(0,0,-12*FMath::Sin(LandingPulse/.24f*PI)));
    if(bStarted&&!bFinished)
    {
        MinArm=FMath::Min(MinArm,CameraBoom->TargetArmLength);MaxArm=FMath::Max(MaxArm,CameraBoom->TargetArmLength);
        MaxAirHeight=FMath::Max(MaxAirHeight,AirHeight);
    }
}

void ASkiRunnerCharacter::FinishRun()
{
    ASkiGameAudio::Event(this,Hearts>0?TEXT("Victory"):TEXT("Defeat"));
    bFinished=true;PresentationTime=0;GetCharacterMovement()->StopMovementImmediately();GetCharacterMovement()->DisableMovement();SkierVisual->SetVisibility(true,true);
    if(auto* Progress=Cast<USkiJourneyProgress>(GetGameInstance()))Progress->Record(ASkiSampleTrail::GetActiveCourse(),Score,Time,TotalFlips,Hearts);
    if(Hearts>0){bJumpHeld=bSpinning=bReturningUpright=false;FallRemaining=0;SetActorLocation(FVector(ASkiSampleTrail::FinishX-180,0,ASkiSampleTrail::HeightAt(ASkiSampleTrail::FinishX-180)+91));}
    UE_LOG(LogTemp,Display,TEXT("SKI_COMPLETE time=%.2f hearts=%d jumps=%d ramps=%d maxSpeed=%.1f air=%.1f arm=%.0f..%.0f"),Time,Hearts,Landings,RampLaunches,TopSpeed,MaxAirHeight,MinArm,MaxArm);
    if(bAutomation)
    {
        const bool bPass=bNoInputTest?Hearts==0:(Hearts>0&&GetProgress()>=.999f&&Landings>=5&&RampLaunches>=3&&TopSpeed>1045&&TopSpeed<=1410.1f&&MaxAirHeight>300&&MaxArm-MinArm>600&&bBrakeVerified&&TotalFlips>=3&&GroundPickups>0&&AirPickups>0&&SprayEmitted>100&&(!bCrashTest||Crashes>0)&&FMath::Abs(GetActorLocation().Y)<.1);
        UE_LOG(LogTemp,Display,TEXT("SKI_FEEDBACK_CHECK maxHearts=%d crashes=%d spray=%d"),MaxHearts,Crashes,SprayEmitted);
        FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"max_hearts\":%d,\"crashes\":%d,\"snow_particles_emitted\":%d,\"crash_test\":%s}"),MaxHearts,Crashes,SprayEmitted,bCrashTest?TEXT("true"):TEXT("false")),*(FPaths::ProjectSavedDir()/TEXT("SkiQA/feedback.json")));
        UE_LOG(LogTemp,Display,TEXT("SKI_BRAKE_TEST verified=%d"),bBrakeVerified);
        FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"no_input_test\":%s,\"hearts\":%d,\"landings\":%d,\"ramps\":%d,\"seconds\":%.2f,\"max_speed_cm_s\":%.1f,\"max_air_height_cm\":%.1f,\"camera_min_cm\":%.1f,\"camera_max_cm\":%.1f,\"max_slope_deg\":%.1f,\"plane_y\":%.4f,\"flips\":%d,\"best_combo\":%d,\"ground_pickups\":%d,\"air_pickups\":%d,\"released_flights\":%d,\"held_flights\":%d}"),bPass?TEXT("true"):TEXT("false"),bNoInputTest?TEXT("true"):TEXT("false"),Hearts,Landings,RampLaunches,Time,TopSpeed,MaxAirHeight,MinArm,MaxArm,MaxSlope,GetActorLocation().Y,TotalFlips,BestCombo,GroundPickups,AirPickups,ReleasedFlights,HoldFlights),*(FPaths::ProjectSavedDir()/(bNoInputTest?TEXT("SkiQA/no_input.json"):TEXT("SkiQA/smoke.json"))));
    }
}
void ASkiRunnerCharacter::Tick(float Dt)
{
    Super::Tick(Dt);
    if(bRunPaused)return;
    UpdateCamera(Dt);
    UpdateSkiSpray(Dt);
    if(bPresentationTest)UpdatePresentationCheck(Dt);
    if(!bStarted){ApplyBodyPose(.08f,0,0,FMath::Sin(PresentationTime*1.6f));return;}
    if(bFinished){if(Hearts<=0)UpdateFallAnimation(Dt);else UpdateVictoryPose();return;}
    Time+=Dt;MessageTime=FMath::Max(0.f,MessageTime-Dt);
    const FVector P=GetActorLocation();auto* M=GetCharacterMovement();
    const float Slope=GetSlopeDegrees();MaxSlope=FMath::Max(MaxSlope,FMath::Abs(Slope));
    const float Feet=P.Z-GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    bool CanLandOnTop=M->IsMovingOnGround()&&M->CurrentFloor.HitResult.Component.IsValid()
        &&M->CurrentFloor.HitResult.Component->GetCollisionObjectType()==ECC_GameTraceChannel1;
    const auto& Tops=ASkiSampleTrail::ObstaclePositions();
    if(M->IsFalling())for(int32 I=0;I<Tops.Num();++I)
        if(ASkiSampleTrail::IsRideableObstacle(I)&&FMath::Abs(P.X-Tops[I])<300
            &&Feet>=ASkiSampleTrail::ObstacleTopAt(I,Tops[I])-8)CanLandOnTop=true;
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1,CanLandOnTop?ECR_Block:ECR_Ignore);
    M->bUseFlatBaseForFloorChecks=!CanLandOnTop;
    if(bAutomation&&!bNoInputTest&&Time<1.8f)
    {
        if(Time>.6f&&Time<1.4f)BrakePressed();else BrakeReleased();
    }
    if(M->IsMovingOnGround())
    {
        // Approach a modest slope-dependent cruising speed instead of building
        // up to the cap on every downhill. Ground speed follows the surface.
        const float TargetSpeed=CruiseSpeedForSlope(Slope,bBraking,Time<BoostUntil);
        ForwardSpeed=FMath::FInterpConstantTo(ForwardSpeed,TargetSpeed,Dt,bBraking?1500.f:TargetSpeed>ForwardSpeed?200.f:320.f);
        if(bBraking&&ForwardSpeed<650)bBrakeVerified=true;
    }
    else AirSeconds+=Dt;
    if(Time<HurtUntil)ForwardSpeed=FMath::Min(ForwardSpeed,FallRemaining>0?420.f:760.f);
    M->MaxWalkSpeed=ForwardSpeed;AddMovementInput(FVector::ForwardVector,1);
    TopSpeed=FMath::Max(TopSpeed,ForwardSpeed);
    DistanceScore=FMath::Max(DistanceScore,(P.X-200)*.01f);
    if(bAutomation&&!bNoInputTest&&IsAirborne())
    {
        // Use the same hold delay and slow return as manual play. Only begin
        // tricks on longer jumps; crash mode deliberately releases too late.
        FVector PredictedLanding;const float Remaining=PredictLanding(PredictedLanding);
        if(bCrashTest&&Crashes==0)
        {
            if(SpinAngle>160&&Remaining<.18f)ReleaseJump();
            else if(!bReleasedThisFlight)bJumpHeld=true;
        }
        else if(!bReleasedThisFlight)
        {
            if(SpinAngle>300&&Remaining<1.4f)ReleaseJump();
            else if(SpinVelocity>0||AirHoldSeconds>0||Remaining>2.25f)bJumpHeld=true;
            else bJumpHeld=false;
        }
    }
    UpdateTricks(Dt);UpdatePickups(Dt);
    SkierVisual->SetVisibility(FallRemaining>0||Time>=HurtUntil||FMath::Fmod(Time*9,1.f)>.35f,true);

    const auto& Ramps=ASkiSampleTrail::RampPositions();
    for(int I=0;I<Ramps.Num();++I)
    {
        if(bAutomation&&!bNoInputTest&&!UsedRamps.Contains(I)&&M->IsMovingOnGround())
        {
            const float Approach=Ramps[I]-P.X;
            if(Approach>100&&Approach<ForwardSpeed*.35f)StartOrJump();
        }
        if(!UsedRamps.Contains(I)&&LastX<Ramps[I]&&P.X>=Ramps[I])
        {
            UsedRamps.Add(I);++RampLaunches;
            if(M->IsMovingOnGround())
            {
                LaunchCharacter(FVector(ForwardSpeed,0,420+ForwardSpeed*.22f),true,true);
                ASkiGameAudio::Event(this,TEXT("Jump"),.8f);
                ShowMessage(TEXT("飞坡腾空！按住空格后空翻，沿冰晶弧线收集"),1.6);
            }
        }
    }
    const auto& Obs=ASkiSampleTrail::ObstaclePositions();
    for(int I=0;I<Obs.Num();++I)
    {
        const float X=Obs[I],Distance=X-P.X;
        if(bAutomation&&!bNoInputTest&&Distance>180&&Distance<ForwardSpeed*.60f&&M->IsMovingOnGround())
        {
            // The long course has uphill as well as downhill obstacles. Test
            // inputs use a predicted crossing height instead of one fixed delay.
            const float Arrival=Distance/FMath::Max(ForwardSpeed,1.f);
            float JumpVelocity=900+ForwardSpeed*.13f+FMath::Clamp(FMath::Tan(FMath::DegreesToRadians(Slope))*ForwardSpeed,0.f,350.f);
            for(float Ramp:ASkiSampleTrail::RampPositions())if(Ramp-P.X>-80&&Ramp-P.X<450)JumpVelocity+=ASkiSampleTrail::RampJumpBonus();
            const float PredictedFeet=Feet+JumpVelocity*Arrival+.5f*M->GetGravityZ()*Arrival*Arrival;
            if(PredictedFeet-ASkiSampleTrail::HeightAt(X)>ASkiSampleTrail::ObstacleHeight(I)+35)StartOrJump();
        }
        // Test the whole crossing interval, including low-frame-rate sweeps.
        if(!PassedObstacles.Contains(I)&&P.X>=X-120&&LastX<=X+120)
        {
            const float SampleX=FMath::Clamp(X,LastX,P.X);
            const float Alpha=FMath::Clamp((SampleX-LastX)/FMath::Max(P.X-LastX,1.f),0.f,1.f);
            const float Clearance=FMath::Lerp(LastFeet,Feet,Alpha)-ASkiSampleTrail::HeightAt(SampleX);
            const bool Rideable=ASkiSampleTrail::IsRideableObstacle(I);
            const float Top=Rideable?ASkiSampleTrail::ObstacleTopAt(I,SampleX):ASkiSampleTrail::HeightAt(SampleX)+ASkiSampleTrail::ObstacleHeight(I)+10;
            const bool OnTop=M->IsMovingOnGround()&&M->CurrentFloor.HitResult.Component.IsValid()
                &&M->CurrentFloor.HitResult.Component->GetCollisionObjectType()==ECC_GameTraceChannel1;
            const bool Safe=Rideable&&(OnTop||FMath::Lerp(LastFeet,Feet,Alpha)>=Top-8);
            // A successful top landing clears this obstacle, including the drop off its rear edge.
            if(Rideable&&OnTop)PassedObstacles.Add(I);
            if(!Safe&&Clearance<ASkiSampleTrail::ObstacleHeight(I)+10)
            {
                PassedObstacles.Add(I);Hurt();
                if(bFinished)return;
            }
            else if(P.X>X+120)PassedObstacles.Add(I);
        }
    }
    if(bAutomation&&!bNoInputTest)
        for(float Landmark : {55000.f,68000.f,80500.f,95500.f})
            if(LastX<Landmark-350&&P.X>=Landmark-350)
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("SkiQA/Culture_%05d.png"),FMath::RoundToInt(Landmark)),true,false);
    LastX=P.X;LastFeet=Feet;
    if(bAutomation&&FMath::FloorToInt(Time/3)!=FMath::FloorToInt((Time-Dt)/3))
        UE_LOG(LogTemp,Display,TEXT("SKI_POSITION t=%.1f x=%.1f z=%.1f vx=%.1f slope=%.1f arm=%.0f mode=%d"),Time,P.X,P.Z,M->Velocity.X,Slope,CameraBoom->TargetArmLength,int(M->MovementMode));
    const int Region=P.X<2500?0:P.X<17000?1:P.X<42000?2:P.X<53000?3:P.X<65500?4:P.X<79000?5:P.X<93000?6:7;
    if(Region!=LastRegion)
    {
        LastRegion=Region;
        if(ASkiSampleTrail::GetActiveCourse()>0)ShowMessage(ASkiSampleTrail::GetActiveCourse()==1?TEXT("北大湖 · 留意飞坡旗，越过软隔离墩"):TEXT("长白岩谷 · 穿行玄武岩与蓝冰，提前观察落地区"),3);
        else if(Region==1)ShowMessage(TEXT("雾凇雪谷 · 陡坡自动加速，按住 S 控速"),3);
        if(ASkiSampleTrail::GetActiveCourse()==0){
        if(Region==2)ShowMessage(TEXT("白山松水 · 连续起伏，别错过金色起跳线"),3);
        if(Region==3)ShowMessage(TEXT("长白山麓 · 越过林海雪坡，收集空中冰晶"),3);
        if(Region==4)ShowMessage(TEXT("查干湖冬捕 · 冰湖、渔网与吉林冬日记忆"),3);
        if(Region==5)ShowMessage(TEXT("延边风情 · 雪中屋檐、打糕与暖暖的人情"),3);
        if(Region==6)ShowMessage(TEXT("长春记忆 · 老电车穿行在城市的冬日故事里"),3);
        if(Region==7)ShowMessage(TEXT("青春相约长春2027 · 让灯火与烟花迎接你"),3);
        }
    }
    if(P.Z<ASkiSampleTrail::HeightAt(P.X)-500||FMath::Abs(P.Y)>5)
    {
        Hurt();if(bFinished)return;
        SetActorLocation(FVector(P.X,0,ASkiSampleTrail::HeightAt(P.X)+110));
        M->StopMovementImmediately();M->SetMovementMode(MOVE_Walking);
    }
    if(bAutomation&&!bNoInputTest&&Time>NextScreenshot&&NextScreenshot>0)
    {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("SkiQA/Frame_%02d.png"),FMath::RoundToInt(NextScreenshot)),true,false);
        NextScreenshot=NextScreenshot<2?5:NextScreenshot<6?10:NextScreenshot<11?18:NextScreenshot<19?28:NextScreenshot<29?42:NextScreenshot<43?56:10000;
    }
    if(P.X>=ASkiSampleTrail::FinishX)FinishRun();
}
