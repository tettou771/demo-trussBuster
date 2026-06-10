# TRUSS BUSTER

トラスタワーを大砲で吹っ飛ばす 3D 物理デモリッションゲーム。
TrussC + tcxPhysics (Jolt) + tcxImGui + tcxNodeInspector のショーケース。

**▶ ブラウザで遊ぶ: https://tettou771.github.io/demo-trussBuster/** （スマホ対応・WebGL2）

![genre](https://img.shields.io/badge/genre-arcade%20demolition-orange)

## 遊び方

プラットフォーム上のブロックを**全部台から叩き落とせばクリア**。弾数制限あり。
レベルは 10。前半5つは物理崩しの王道（FIRST CONTACT → PYRAMID → THE WALL →
TWIN TOWERS → TRUSS CASTLE）、後半5つは**壊す順番が鍵のパズル面**
（NEEDLE EYE → THE PLOW → DOMINO RUN → DOUBLE TAP → MASTERPIECE）。
壁を横倒しにしてからプラウのように押し込む、密着ペアの継ぎ目を撃って2個同時に落とす、
ドミノの先頭だけを倒す — など。

| キー | 操作 |
|------|------|
| ENTER | スタート |
| ← → ↑ ↓ | 照準（ヨー / ピッチ） |
| SPACE 長押し→離す | パワーゲージが往復、離した位置で発射 |
| F1 | ノードインスペクタ (tcxNodeInspector) |
| F2 | デバッグパネル (ImGui) |

- **MAXショット**: ゲージが往復する頂点（白いゾーン、93%以上）でジャスト離すと
  強化ショット。頂点でチック音が鳴るので耳でもタイミングが取れる
- スコア: 通常ブロック 100 / 梁 150-300 / 金ブロック 500
- クリア時ボーナス: 残弾 × 200
- タイトル画面では CPU がアトラクトデモをプレイする（CPUは8割パワー縛り）

### スマホ（タッチ操作）

スマホ／タブレット（ネイティブ or モバイルブラウザの wasm）では自動判定で
タッチUIに切り替わる: 半透明の十字キーが左下、FIREボタンが右下、タップでスタート。
判定は `src/Mobile.h`（ネイティブ = `Platform::isMobile()`、wasm = UA +
`maxTouchPoints`）。デスクトップで試すには `TB_FORCE_TOUCH=1` で起動。

## ビルド & 実行

```bash
trusscli run            # native
trusscli build --web    # wasm -> bin/trussBuster.{html,js,wasm,data}
cd bin && python3 -m http.server 8888   # スマホからは http://<LAN IP>:8888/trussBuster.html
```

## AI Automation (MCP)

```bash
TRUSSC_MCP=1 TRUSSC_MCP_PORT=8765 ./bin/trussBuster.app/Contents/MacOS/trussBuster
```

標準ツール（screenshot / node tree / input / ImGui 操作）に加え、ゲーム専用ツール:

| Tool | 説明 |
|------|------|
| `get_state` | フェーズ・レベル・スコア・残弾・残ブロック・砲の状態を JSON で返す |
| `set_aim {yaw, pitch}` | 照準セット（度、yaw + が左） |
| `fire {power}` | 発射 (0-1) |
| `press_start` | タイトルからスタート / リザルトからタイトルへ |
| `goto_level {level}` | レベルジャンプ（デバッグ用） |
| `set_autopilot {on}` | CPU プレイの ON/OFF |
| `load_custom_level {level}` | JSONレイアウトをレベル1として即ロード（ステージ設計用、リビルド不要） |

CPU は弾道方程式（2 パス、砲口位置補正つき）でブロックを狙う。
低い段を優先してタワーを崩し、残り 2 個以下ではノイズなしのスナイパーモード。

## 実装メモ

- 効果音・BGM は全て `ChipSound` で実行時生成（アセットファイルなし）。
  BGM は A マイナー 132BPM の 8 小節ループ（矩形波リード + 三角波ベース + ノイズハット）
- ブロックは `RigidBody` Mod + `FlatRenderer` Mod を持つ自己完結 Node
  （台から落ちる = 中心が PLATFORM_TOP - 0.38 を下回ると bust）
- 描画は GPU PBR ではなく **CPU ライティング**（`Mesh::drawWithLighting`、sokol_gl 経由）。
  meshPbr シェーダが iOS Safari で GPU コンテキストを落とす問題の回避と、
  deferred PBR が HUD テキストを上書きする問題の回避を兼ねる（`FlatRenderer.h` 参照）
- HUD はビットマップフォントのみ（レトロ表現）
- カメラは固定（EasyCam のマウス入力は無効）。EasyCam の orbit は左クリックを
  consume するので、有効にするとタッチボタン等にクリックが届かなくなる点に注意
