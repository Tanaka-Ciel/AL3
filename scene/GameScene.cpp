#include "GameScene.h"
#include "MathUtilityForText.h"
#include "TextureManager.h"
#include "time.h"
#include <cassert>

GameScene::GameScene() {}

GameScene::~GameScene() {
	delete spriteBG_;    // BG
	delete modelStage_;  // ステージ
	delete modelPlayer_; // プレイヤー
	delete modelBeam_;   // ビーム
	delete modelEnemy_;  // 敵
}

void GameScene::Initialize() {

	dxCommon_ = DirectXCommon::GetInstance();
	input_ = Input::GetInstance();
	audio_ = Audio::GetInstance();

	// BG(2Dスプライト)
	textureHandleBG_ = TextureManager::Load("bg.jpg");
	spriteBG_ = Sprite::Create(textureHandleBG_, {0, 0});

	// ビュープロジェクションの初期化
	viewProjection_.Initialize();
	viewProjection_.translation_.y = 1;
	viewProjection_.translation_.z = -6;
	viewProjection_.Initialize();

	// ステージ
	textureHandleStage_ = TextureManager::Load("stage.jpg");
	modelStage_ = Model::Create();
	worldTransformStage_.Initialize();

	// ステージの位置を変更
	worldTransformStage_.translation_ = {0, -1.5f, 0};
	worldTransformStage_.scale_ = {4.5f, 1, 40};

	// 変換行列を更新
	worldTransformStage_.matWorld_ = MakeAffineMatrix(
	    worldTransformStage_.scale_, worldTransformStage_.rotation_,
	    worldTransformStage_.translation_);

	// 変換行列を定数バッファに転送
	worldTransformStage_.TransferMatrix();

	// プレイヤー
	textureHandlePlayer_ = TextureManager::Load("player.png");
	modelPlayer_ = Model::Create();
	worldTransformPlayer_.scale_ = {0.5f, 0.5f, 0.5f};
	worldTransformPlayer_.Initialize();

	// ビーム
	textureHandleBeam_ = TextureManager::Load("beam.png");
	modelBeam_ = Model::Create();
	worldTransformBeam_.scale_ = {0.2f, 0.2f, 0.2f};
	worldTransformBeam_.Initialize();

	// 敵
	textureHandleEnemy_ = TextureManager::Load("enemy.png");
	modelEnemy_ = Model::Create();
	worldTransformEnemy_.scale_ = {0.5f, 0.5f, 0.5f};
	worldTransformEnemy_.Initialize();
	srand((unsigned int)time(NULL));

	// デバッグテキスト
	debugText_ = DebugText::GetInstance();
	debugText_->Initialize();
}

void GameScene::Update() {
	PlayerUpdate(); // プレイヤー更新
	BeamUpdate();   // ビーム更新
	EnemyUpdate();  // 敵更新
	Collision();    // 衝突判定
}

//--------------------------------------------------------------------
// プレイヤー
//--------------------------------------------------------------------
void GameScene::PlayerUpdate() {
	// プレイヤー更新
	worldTransformPlayer_.matWorld_ = MakeAffineMatrix(
	    worldTransformPlayer_.scale_, worldTransformPlayer_.rotation_,
	    worldTransformPlayer_.translation_);

	// 変換行列を定数バッファに転送
	worldTransformPlayer_.TransferMatrix();

	// 移動

	// 右へ移動
	if (input_->PushKey(DIK_D) && beamFlag_ == false) {
		// ビーム未発生時:プレイヤーとビームの座標同一
		worldTransformPlayer_.translation_.x += 0.1f;
		worldTransformBeam_.translation_.x += 0.1f;
	} else if (input_->PushKey(DIK_D) && beamFlag_) {
		// ビーム発生中:ビームのみ直進、プレイヤーは入力された動作
		worldTransformPlayer_.translation_.x += 0.1f;
	}

	// 左へ移動
	if (input_->PushKey(DIK_A) && beamFlag_ == false) {
		// ビーム未発生時:プレイヤーとビームの座標同一
		worldTransformPlayer_.translation_.x -= 0.1f;
		worldTransformBeam_.translation_.x -= 0.1f;
	} else if (input_->PushKey(DIK_A) && beamFlag_) {
		// ビーム発生中:ビームのみ直進、プレイヤーは入力された動作
		worldTransformPlayer_.translation_.x -= 0.1f;
	}

	// 移動制限

	// 右側への移動制限
	if (worldTransformPlayer_.translation_.x >= 4) {
		worldTransformPlayer_.translation_.x = 4;
		worldTransformBeam_.translation_.x = 4;
	}

	// 左側への移動制限
	if (worldTransformPlayer_.translation_.x <= -4) {
		worldTransformPlayer_.translation_.x = -4;
		worldTransformBeam_.translation_.x = -4;
	}
}

//----------------------------------------------------------------
// ビーム
//----------------------------------------------------------------
void GameScene::BeamUpdate() {

	BeamMove(); // ビーム移動
	BeamBorn(); // ビーム発生
	// ビーム更新
	worldTransformBeam_.matWorld_ = MakeAffineMatrix(
	    worldTransformBeam_.scale_, worldTransformBeam_.rotation_,
	    worldTransformBeam_.translation_);

	// 変換行列を定数バッファに転送
	worldTransformBeam_.TransferMatrix();
}

// ビーム移動
void GameScene::BeamMove() {
	if (beamFlag_) {
		worldTransformBeam_.translation_.z += 1.0f;
		worldTransformBeam_.rotation_.x += 0.1f;
	}
}

// ビーム発生
void GameScene::BeamBorn() {
	if (input_->PushKey(DIK_SPACE)) {
		beamFlag_ = true;
		worldTransformBeam_.translation_.z = worldTransformPlayer_.translation_.z;
		worldTransformBeam_.translation_.x = worldTransformPlayer_.translation_.x;
	}

	// ビームのZ座標が40以上になったらビームが消える処理
	if (worldTransformBeam_.translation_.z >= 40) {
		beamFlag_ = false;
	}
}

//------------------------------------------------------------------
// 敵
//------------------------------------------------------------------
void GameScene::EnemyUpdate() {
	// 敵更新
	worldTransformEnemy_.matWorld_ = MakeAffineMatrix(
	    worldTransformEnemy_.scale_, worldTransformEnemy_.rotation_,
	    worldTransformEnemy_.translation_);

	// 変換行列を定数バッファに転送
	worldTransformEnemy_.TransferMatrix();

	EnemyBorn();
	EnemyMove();
}

// 敵移動
void GameScene::EnemyMove() {
	if (enemyFlag_) {
		worldTransformEnemy_.translation_.z -= 0.5f;
		worldTransformEnemy_.rotation_.x -= 0.1f;
	}
}

// 敵発生
void GameScene::EnemyBorn() {
	if (enemyFlag_ == false) {
		enemyFlag_ = true;
		worldTransformEnemy_.translation_.z = 40;

		// 乱数でX座標の指定
		int x = rand() % 80;          // 80は4の10倍の2倍
		float x2 = (float)x / 10 - 4; // 10で割り、4を引く
		worldTransformEnemy_.translation_.x = x2;
	}

	// 敵のZ座標が-4以下になったら敵が発生源に戻る処理
	if (worldTransformEnemy_.translation_.z <= -5) {
		enemyFlag_ = false;
	}
}

//-------------------------------------------------------------------------------------------------
// 衝突判定
//-------------------------------------------------------------------------------------------------
void GameScene::Collision() {

	// 衝突判定(プレイヤーと敵)
	CollisionPlayerEnemy();
	// 衝突判定(敵とビーム)
	CollisionBeamEnemy();
}

void GameScene::CollisionPlayerEnemy() {

	// 敵が存在すれば
	if (enemyFlag_) {

		// 差を求める
		float dx = abs(worldTransformPlayer_.translation_.x - worldTransformEnemy_.translation_.x);
		float dz = abs(worldTransformPlayer_.translation_.z - worldTransformEnemy_.translation_.z);

		// 衝突したら
		if (dx < 1 && dz < 1) {

			// 存在しない
			enemyFlag_ = false;
		}
	}
}

void GameScene::CollisionBeamEnemy() {

	// 敵が存在すれば
	if (enemyFlag_ && beamFlag_) {

		// 差を求める
		float dx = abs(worldTransformBeam_.translation_.x - worldTransformEnemy_.translation_.x);
		float dz = abs(worldTransformBeam_.translation_.z - worldTransformEnemy_.translation_.z);

		// 衝突したら
		if (dx < 1 && dz < 1) {

			// 存在しない
			enemyFlag_ = false;
			beamFlag_ = false;
		}
	}
}

void GameScene::Draw() {

	// コマンドリストの取得
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

#pragma region 背景スプライト描画
	// 背景スプライト描画前処理
	Sprite::PreDraw(commandList);

	/// <summary>
	/// ここに背景スプライトの描画処理を追加できる

	// 背景
	spriteBG_->Draw();

	/// </summary>

	// スプライト描画後処理
	Sprite::PostDraw();
	// 深度バッファクリア
	dxCommon_->ClearDepthBuffer();
#pragma endregion

#pragma region 3Dオブジェクト描画
	// 3Dオブジェクト描画前処理
	Model::PreDraw(commandList);

	/// <summary>
	/// ここに3Dオブジェクトの描画処理を追加できる

	// ステージ
	modelStage_->Draw(worldTransformStage_, viewProjection_, textureHandleStage_);

	// プレイヤー
	modelPlayer_->Draw(worldTransformPlayer_, viewProjection_, textureHandlePlayer_);

	// ビーム
	if (beamFlag_) {
		modelBeam_->Draw(worldTransformBeam_, viewProjection_, textureHandleBeam_);
	}

	// 敵
	if (enemyFlag_) {
		modelEnemy_->Draw(worldTransformEnemy_, viewProjection_, textureHandleEnemy_);
	}

	/// </summary>

	// 3Dオブジェクト描画後処理
	Model::PostDraw();
#pragma endregion

#pragma region 前景スプライト描画
	// 前景スプライト描画前処理
	Sprite::PreDraw(commandList);

	/// <summary>
	/// ここに前景スプライトの描画処理を追加できる

	debugText_->Print("AAA", 10, 10, 2);
	debugText_->DrawAll();

	// ゲームスコア
	char str[100];
	sprintf_s(str, "SCORE %d", gameScore_);
	debugText_->Print(str, 200, 10, 2);

	/// </summary>

	// スプライト描画後処理
	Sprite::PostDraw();

#pragma endregion
}
