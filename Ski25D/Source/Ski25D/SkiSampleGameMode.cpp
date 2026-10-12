#include "SkiSampleGameMode.h"
#include "SkiGameAudio.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "SkiRunnerCharacter.h"
#include "SkiJourneyAtmosphere.h"
#include "SkiSampleTrail.h"
#include "SkiJourneyProgress.h"
#include "HAL/FileManager.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/GameViewportClient.h"
#include "Misc/FileHelper.h"
#include "CanvasItem.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformMisc.h"
#include "Fonts/SlateFontInfo.h"
#include "Kismet/GameplayStatics.h"

ASkiSampleGameMode::ASkiSampleGameMode()
{
    DefaultPawnClass=ASkiRunnerCharacter::StaticClass();HUDClass=ASkiSampleHUD::StaticClass();
    PrimaryActorTick.bCanEverTick=true;
}
void ASkiSampleGameMode::InitGame(const FString& MapName,const FString& Options,FString& ErrorMessage)
{
    ASkiSampleTrail::SetActiveCourse(MapName.Contains(TEXT("Beidahu"))?1:MapName.Contains(TEXT("Changbai"))?2:0);
    Super::InitGame(MapName,Options,ErrorMessage);
    UE_LOG(LogTemp,Display,TEXT("SKI_CHAPTER_BEGIN chapter=%d map=%s"),ASkiSampleTrail::GetActiveCourse()+1,*MapName);
}
void ASkiSampleGameMode::BeginPlay()
{
    Super::BeginPlay();
    GetWorld()->SpawnActor<ASkiGameAudio>();
    GetWorld()->SpawnActor<ASkiJourneyAtmosphere>();
}
void ASkiSampleGameMode::Tick(float Dt)
{
    Super::Tick(Dt);
    if(RunFinaleChecks(Dt))return;
    if(RunExperienceChecks(Dt))return;
    if(FParse::Param(FCommandLine::Get(),TEXT("SkiRideTest")))
    {
        auto* PC=GetWorld()->GetFirstPlayerController();auto* P=PC?Cast<ASkiRunnerCharacter>(PC->GetPawn()):nullptr;if(!P)return;
        const auto& Obstacles=ASkiSampleTrail::ObstaclePositions();int32 Index=0;
        while(Index<Obstacles.Num()&&!ASkiSampleTrail::IsRideableObstacle(Index))++Index;
        if(Index>=Obstacles.Num()){FPlatformMisc::RequestExit(false);return;}
        static int32 GroundHearts=0,TopFrames=0;static float FirstTopX=0,SlideDistance=0;
        const float X=Obstacles[Index];QASeconds+=Dt;
        if(PreviewStep<0){PreviewStep=0;P->PreviewCoursePoint((X-1100)/(ASkiSampleTrail::FinishX-3700));}
        auto* Movement=P->GetCharacterMovement();
        if(PreviewStep==0&&P->GetActorLocation().X>X+350)
        {
            GroundHearts=P->GetHearts();PreviewStep=1;P->RestartRun();
            P->PreviewCoursePoint((X-400)/(ASkiSampleTrail::FinishX-3700));
            P->SetActorLocation(FVector(X-200,0,ASkiSampleTrail::ObstacleTopAt(Index,X)+88+130));
            Movement->SetMovementMode(MOVE_Falling);Movement->Velocity=FVector(760,0,-450);
        }
        const bool Supported=PreviewStep==1&&Movement->IsMovingOnGround()&&Movement->CurrentFloor.HitResult.Component.IsValid()
            &&Movement->CurrentFloor.HitResult.Component->GetCollisionObjectType()==ECC_GameTraceChannel1;
        if(Supported)
        {
            if(TopFrames==0)FirstTopX=P->GetActorLocation().X;
            ++TopFrames;SlideDistance=P->GetActorLocation().X-FirstTopX;
            if(!bPreviewCaptured)
            {
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("SkiQA/Rideable_%d.png"),ASkiSampleTrail::GetActiveCourse()+1),true,false);bPreviewCaptured=true;
            }
        }
        if((PreviewStep==1&&P->GetActorLocation().X>X+750)||QASeconds>10)
        {
            const bool Pass=GroundHearts==4&&P->GetHearts()==5&&TopFrames>=2&&SlideDistance>5&&P->GetActorLocation().X>X+750;
            FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"ground_hit_hearts\":%d,\"top_landing_hearts\":%d,\"supported_frames\":%d,\"slide_cm\":%.2f}"),Pass?TEXT("true"):TEXT("false"),GroundHearts,P->GetHearts(),TopFrames,SlideDistance),*(FPaths::ProjectSavedDir()/FString::Printf(TEXT("SkiQA/rideable_%d.json"),ASkiSampleTrail::GetActiveCourse()+1)));
            UE_LOG(LogTemp,Display,TEXT("SKI_RIDE_CHECK passed=%d ground=%d top=%d frames=%d slide=%.1f"),Pass,GroundHearts,P->GetHearts(),TopFrames,SlideDistance);
            FPlatformMisc::RequestExit(false);
        }
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("SkiMenuReview")))
    {
        QASeconds+=Dt;auto* PC=GetWorld()->GetFirstPlayerController();auto* P=PC?Cast<ASkiRunnerCharacter>(PC->GetPawn()):nullptr;
        if(!P)return;
        const int32 Step=FMath::Min(int32(QASeconds/3),5);
        if(Step!=PreviewStep)
        {
            PreviewStep=Step;bPreviewCaptured=false;
            if(Step==0)
            {
                P->SelectedCourse=0;P->MenuLeft();const bool Wrap=P->SelectedCourse==2;P->MenuRight();const bool Back=P->SelectedCourse==0;
                P->MenuAction(5);P->MenuDown();P->MenuConfirm();const bool Select=P->MenuPage==1;
                P->MenuAction(5);P->MenuDown();P->MenuDown();P->MenuConfirm();const bool History=P->MenuPage==2;
                P->MenuAction(5);P->SelectedCourse=ASkiSampleTrail::GetActiveCourse();P->MenuConfirm();const bool Start=P->HasStarted();P->RestartRun();
                auto* Save=Cast<USkiJourneySave>(UGameplayStatics::CreateSaveGameObject(USkiJourneySave::StaticClass()));
                FSkiHistoryEntry E;E.Chapter=2;E.Score=2850;E.Seconds=178;E.Flips=8;E.Hearts=4;E.Date=TEXT("2026.10.11  12:00");Save->Entries.Add(E);
                const FString Slot=TEXT("SkiMenuQATemporary");const bool Written=UGameplayStatics::SaveGameToSlot(Save,Slot,0);
                auto* Loaded=Cast<USkiJourneySave>(UGameplayStatics::LoadGameFromSlot(Slot,0));
                const bool Disk=Written&&Loaded&&Loaded->Entries.Num()==1&&Loaded->Entries[0].Score==2850&&Loaded->Entries[0].Chapter==2;
                UGameplayStatics::DeleteGameInSlot(Slot,0);
                if(auto* G=Cast<USkiJourneyProgress>(GetGameInstance()))G->History=Save->Entries;
                FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"left_wrap\":%s,\"right_return\":%s,\"scene_menu\":%s,\"history_menu\":%s,\"start\":%s,\"disk_roundtrip\":%s}"),Wrap&&Back&&Select&&History&&Start&&Disk?TEXT("true"):TEXT("false"),Wrap?TEXT("true"):TEXT("false"),Back?TEXT("true"):TEXT("false"),Select?TEXT("true"):TEXT("false"),History?TEXT("true"):TEXT("false"),Start?TEXT("true"):TEXT("false"),Disk?TEXT("true"):TEXT("false")),*(FPaths::ProjectSavedDir()/TEXT("SkiQA/menu_checks.json")));
            }
            if(Step==1)
            {
                int32 VW,VH;PC->GetViewportSize(VW,VH);const float S=VH/900.f;
                const bool Click=PC->GetHUD()->UpdateAndDispatchHitBoxClickEvents(FVector2D(150*S,545*S),IE_Pressed);
                ensureAlways(Click&&P->MenuPage==1);P->MenuRight();
            }
            if(Step==2)
            {
                int32 VW,VH;PC->GetViewportSize(VW,VH);const float S=VH/900.f;
                const bool Click=PC->GetHUD()->UpdateAndDispatchHitBoxClickEvents(FVector2D(150*S,645*S),IE_Pressed);
                ensureAlways(Click&&P->MenuPage==2);
            }
            if(Step==3)P->PreviewVictory();
        }
        if(!bPreviewCaptured&&FMath::Fmod(QASeconds,3.f)>(Step==4?1.3f:2.5f))
        {
            const TCHAR* Names[]={TEXT("Home"),TEXT("SceneSelector"),TEXT("History"),TEXT("VictoryApproach"),TEXT("VictoryCheer"),TEXT("VictoryMenu")};
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("SkiQA/%s.png"),Names[Step]),true,false);bPreviewCaptured=true;
            if(Step==3)ensureAlways(P->IsCelebrating());
            if(Step==5)ensureAlways(!P->IsCelebrating());
        }
        if(QASeconds>18)FPlatformMisc::RequestExit(false);return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("SkiPreviewTest")))
    {
        QASeconds+=Dt;
        auto* PC=GetWorld()->GetFirstPlayerController();auto* Runner=PC?Cast<ASkiRunnerCharacter>(PC->GetPawn()):nullptr;
        const int32 Step=FMath::Min(FMath::FloorToInt(QASeconds/3.f),3);
        if(Runner&&Step!=PreviewStep)
        {
            PreviewStep=Step;bPreviewCaptured=false;Runner->RestartRun();
            const float Fractions[]={.02f,.27f,.60f,.92f};
            if(FParse::Param(FCommandLine::Get(),TEXT("SkiSignReview")))
            {
                const float SignX[]={900,ASkiSampleTrail::GetActiveCourse()==2?36200.f:35800.f,ASkiSampleTrail::GetActiveCourse()==2?113500.f:87500.f,ASkiSampleTrail::FinishX-1700};
                Runner->PreviewCoursePoint((SignX[Step]-2200)/(ASkiSampleTrail::FinishX-3700));
            }
            else Runner->PreviewCoursePoint(Fractions[Step]);
        }
        if(!bPreviewCaptured&&FMath::Fmod(QASeconds,3.f)>1.8f)
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("SkiQA/%sChapter_%d_View_%d.png"),FParse::Param(FCommandLine::Get(),TEXT("SkiArtPreview"))?TEXT("Art_"):TEXT(""),ASkiSampleTrail::GetActiveCourse()+1,Step+1),true,false);bPreviewCaptured=true;
        }
        if(QASeconds>12)FPlatformMisc::RequestExit(false);
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("SkiSmokeTest"))||FParse::Param(FCommandLine::Get(),TEXT("SkiNoInputTest")))
    {
        const bool NoInput=FParse::Param(FCommandLine::Get(),TEXT("SkiNoInputTest"));
        const FString Report=FPaths::ProjectSavedDir()/(NoInput?TEXT("SkiQA/no_input.json"):TEXT("SkiQA/smoke.json"));
        QASeconds+=Dt;
        auto* Controller=GetWorld()->GetFirstPlayerController();
        auto* Runner=Controller?Cast<ASkiRunnerCharacter>(Controller->GetPawn()):nullptr;
        const float PreviousEnd=QAEndSeconds;
        if(Runner&&Runner->HasFinished())QAEndSeconds+=Dt;
        if(!NoInput&&PreviousEnd<1&&QAEndSeconds>=1)
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SkiQA/Finish.png"),true,false);
        if(QASeconds>260||QAEndSeconds>6.8f)
        {
            if(!FPaths::FileExists(Report))
                FFileHelper::SaveStringToFile(TEXT("{\"passed\":false,\"reason\":\"route_did_not_finish\"}"),*Report);
            const int32 Chapter=ASkiSampleTrail::GetActiveCourse();
            IFileManager::Get().Copy(*(FPaths::ProjectSavedDir()/FString::Printf(TEXT("SkiQA/chapter_%d_smoke.json"),Chapter+1)),*Report);
            IFileManager::Get().Copy(*(FPaths::ProjectSavedDir()/FString::Printf(TEXT("SkiQA/chapter_%d_environment.json"),Chapter+1)),*(FPaths::ProjectSavedDir()/TEXT("SkiQA/environment.json")));
            if(FParse::Param(FCommandLine::Get(),TEXT("SkiCampaignTest"))&&Runner&&Runner->HasFinished()&&Runner->GetHearts()>0&&Chapter<2)
            {
                Runner->StartOrJump();return;
            }
            FPlatformMisc::RequestExit(false);
        }
    }
}
void ASkiSampleHUD::Text(const FString& S,float X,float Y,float Size,FLinearColor C)
{
    const float Scale=Canvas->ClipY/900.f;
    const FSlateFontInfo Font(GEngine->GetMediumFont(),Size*Scale);
    FCanvasTextItem Item(FVector2D(X*Scale,Y*Scale),FText::FromString(S),Font,C);
    Item.EnableShadow(FLinearColor(0,0,0,.25));Canvas->DrawItem(Item);
}
void ASkiSampleHUD::Panel(float X,float Y,float W,float H,FLinearColor C)
{
    const float S=Canvas->ClipY/900.f;
    FCanvasTileItem Item(FVector2D(X*S,Y*S),FVector2D(W*S,H*S),C);
    Item.BlendMode=SE_BLEND_Translucent;Canvas->DrawItem(Item);
}
void ASkiSampleHUD::DrawHUD()
{
    Super::DrawHUD();auto* P=Cast<ASkiRunnerCharacter>(GetOwningPawn());if(!P||!Canvas)return;
    if(FParse::Param(FCommandLine::Get(),TEXT("SkiArtPreview")))return;
    const float W=Canvas->ClipX/(Canvas->ClipY/900.f);
    const bool MenuInput=!P->HasStarted()||(P->HasFinished()&&!P->IsCelebrating());
    auto* PC=GetOwningPlayerController();
    if(PC->bShowMouseCursor!=MenuInput)
    {
        PC->bShowMouseCursor=MenuInput;PC->bEnableClickEvents=MenuInput;
        if(MenuInput){FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);PC->SetInputMode(Mode);}
        else PC->SetInputMode(FInputModeGameOnly());
    }
    if(!P->HasStarted()){DrawMenu(P,W);return;}
    if(P->IsCelebrating())
    {
        Panel(0,0,W,64,FLinearColor(0,0,0,.85));Panel(0,805,W,95,FLinearColor(0,0,0,.85));
        Text(TEXT("抵达终点 · 为自己喝彩"),W*.5f-190,827,26,FLinearColor(.95,.89,.70));return;
    }
    const FLinearColor Ink(.025,.075,.10,.90),White(.93,.98,1),Gold(.96,.65,.27),Muted(.61,.76,.80);
    Panel(28,28,310,94,Ink);Panel(28,28,4,94,Gold);
    Text(TEXT("吉雪奇旅"),48,37,28,White);Text(TEXT("JILIN  /  WINTER JOURNEY"),49,82,12,Muted);
    Panel(W-314,28,286,94,Ink);Text(FString::Printf(TEXT("旅程 %03d   ·   生命 %d / %d"),P->GetScore(),P->GetHearts(),ASkiRunnerCharacter::MaxHearts),W-292,45,17,White);
    Panel(W-292,87,242,4,FLinearColor(.2,.3,.34,1));Panel(W-292,87,242*P->GetProgress(),4,Gold);
    Panel(W-314,137,286,145,Ink);
    Text(FString::Printf(TEXT("%02.0f km/h    落差 %.0f m"),P->GetSpeedKmh(),P->GetDropMetres()),W-292,150,20,White);
    Text(P->HasFinished()&&P->GetHearts()==0?TEXT("本次旅程结束"):P->IsRecovering()?TEXT("摔倒滑行 · 正在起身"):P->IsAirborne()?(P->IsReturningUpright()?TEXT("就近收势 · 缓慢回正"):P->IsFlipping()?TEXT("后空翻 · 松手回正"):TEXT("先腾空 · 长按才空翻")):P->IsBraking()?TEXT("刹车控速"):FMath::Abs(P->GetSlopeDegrees())>25?TEXT("顺坡滑行 · 小幅加速"):TEXT("保持节奏 · 准备起跳"),W-292,193,15,Gold);
    Text(FString::Printf(TEXT("收集 %d    空翻 %d"),P->GetCollected(),P->GetTotalFlips()),W-292,239,16,White);
    Panel(28,774,420,94,Ink);Text(FString::Printf(TEXT("%02d / %s"),ASkiSampleTrail::GetActiveCourse()+1,*P->GetRegion()),48,785,18,White);
    Text(TEXT("空格跳跃/空翻  S刹车  ESC暂停  R重来  M声音"),48,826,11,Muted);
    Text(TEXT("欢迎相约吉林长春 · 共赴冰雪盛会"),W-500,814,15,Gold);
    Text(TEXT("长春2027第33届世界大学生冬季运动会"),W-500,841,15,White);
    const float Clock=9+FMath::Min(P->GetRunSeconds()*(ASkiSampleTrail::GetActiveCourse()==1?.55f:ASkiSampleTrail::GetActiveCourse()==2?.8f:1.f)/55.f,1.f)*13;
    Text(FString::Printf(TEXT("%02d:%02d  %s"),FMath::FloorToInt(Clock),FMath::FloorToInt(FMath::Frac(Clock)*60),Clock<16?TEXT("日光雪旅"):Clock<19?TEXT("暮色将至"):TEXT("雪夜灯火")),W-292,295,16,White);
    if(P->GetPendingFlips()>0&&!P->HasFinished())
        Text(FString::Printf(TEXT("后空翻 ×%d · 落地结算"),P->GetPendingFlips()),W*.5f-130,245,22,Gold);
    if(P->GetRewardTime()>0&&!P->HasFinished())
        Text(P->GetRewardMessage(),W*.5f-150,305,22,Gold);
    if(P->GetMessageTime()>0&&P->HasStarted()&&!P->HasFinished())
    {
        Panel(W*.5f-330,140,660,52,Ink);Text(P->GetMessage(),W*.5f-310,151,17,White);
    }
    if(!P->HasStarted()||P->HasFinished()||P->IsRunPaused())
    {
        const float X=W*.5f-300;
        const bool Dead=P->HasFinished()&&P->GetHearts()==0;
        Panel(X,245,600,366,Dead?FLinearColor(.10,.025,.035,.92):FLinearColor(.015,.05,.075,.94));Panel(X,245,600,3,Dead?FLinearColor(.75,.28,.25):Gold);
        Text(FString::Printf(TEXT("%02d / 03 · %s"),ASkiSampleTrail::GetActiveCourse()+1,*ASkiSampleTrail::CourseTitle()),X+36,274,16,Gold);
        FString Title=!P->HasStarted()?TEXT("循着雪光，遇见吉林"):P->IsRunPaused()?TEXT("歇一会，再出发"):P->GetHearts()>0?TEXT("这一程，收进冬日记忆"):TEXT("调整节奏，再试一次");
        Text(Title,X+36,320,29,White);
        if(!P->HasStarted())
        {
            const int32 Chapter=ASkiSampleTrail::GetActiveCourse();
            Text(Chapter==0?TEXT("雾凇江岸 / 雪村长坡 / 跳跃热身"):Chapter==1?TEXT("开阔雪场 / 缆车飞坡 / 北大湖挑战"):TEXT("长白林海 / 山麓岩壁 / 倒木与冰雪"),X+36,381,18,Muted);
            Text(TEXT("轻按起跳，长按空翻；松手调整落地姿态。"),X+36,425,15,White);
            Text(TEXT("选关：1 雾凇江岸 / 2 北大湖 / 3 长白林海"),X+36,463,15,Gold);
        }
        else if(P->HasFinished())
        {
            Text(FString::Printf(TEXT("探索分数  %d      成功落地  %d"),P->GetScore(),P->GetLandings()),X+36,388,22,White);
            Text(FString::Printf(TEXT("冰雪收集 %d    /    成功后空翻 %d"),P->GetCollected(),P->GetTotalFlips()),X+36,449,17,Gold);
            if(auto* Progress=Cast<USkiJourneyProgress>(GetGameInstance()))Text(FString::Printf(TEXT("三关旅行成绩：%d    ·    R 重试本关"),Progress->Total()),X+36,487,15,Muted);
        }
        else Text(TEXT("按 ESC 继续，或按 R 从起点重新出发。"),X+36,400,18,White);
        Panel(X+36,527,528,48,FLinearColor(.80,.43,.13,1));
        Text(P->IsRunPaused()?TEXT("ESC   继续旅程"):P->HasFinished()?(P->GetHearts()>0?(ASkiSampleTrail::GetActiveCourse()<2?TEXT("SPACE   前往下一关"):TEXT("SPACE   三关完成，重游吉林")):TEXT("SPACE / R   重试本关")):TEXT("SPACE / ENTER   开始滑行"),X+65,535,18,White);
        if(P->HasFinished())
        {
            const float S=Canvas->ClipY/900.f;AddHitBox(FVector2D((X+36)*S,527*S),FVector2D(528*S,48*S),TEXT("0"),true);
            Button(5,TEXT("返回首页"),TEXT(""),X,635,280,54);Button(6,TEXT("再滑一次"),TEXT(""),X+320,635,280,54);
        }
    }
}
