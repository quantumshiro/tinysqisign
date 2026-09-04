# 再構成用パッチとbundle

- `v2-d13.bundle`: Compact-SQIsign基準`5b94b09a...`から凍結commit`71099e08...`までの履歴。SHA-256 `3382af62c2299c1882f55c3520edd6e1b03c4b4f66eb5bdd869f071183cda6bc`。
- `v2-lowmem-d13.patch`: 同じv2変更の閲覧用統合差分。適用後treeは`8761bccb5b14172e21d7228878fb3fc9379db5c4`。SHA-256 `7f24cbcac88abdc1067ea17c524caab53d8307d94985b7d3d2a3fc785f1d2227`。
- `v3-lifetime-overlays.bundle`: 公式v3基準`6d017708...`を前提に、clean commit`9293313f...`を再構成する薄いbundle。treeは`30606e0b5cb2a99d782f4eb334c0c3b87b1edd1c`。SHA-256 `823e3b0dc251646046ba32b5b26a30e8a6c37197ff1a8192f725080e5d13dde2`。
- `v3-two-function-lifetime.bundle`: 同じ基準から二関数D3 commit`874658c6...`を再構成する薄いbundle。treeは`71e3edec18ec40ff4bc315b596c94422f68888d7`。SHA-256 `bd04982daccd3ac25d69a3df2501bc964e9ff4681ec4d03e5fe9532c823a428e`。
- `v3-static-stack.bundle`: 同じ基準から固定frame commit`cb94f242...`を再構成する薄いbundle。treeは`d53293e903e4cb0e5766edc0b3d4b74a3fec6a59`。SHA-256 `a7aa0c4d78507606ded4cd2174898882c004ab4ca34ee6320dfd04837d0cd710`。
- `0035-experiment-v3-d1-lifetime-overlays.patch`: lifetime版の閲覧用差分。`v3-lifetime-overlays.patch`とbyte単位で同一。SHA-256 `44e089298a330b7019b8d9c82110b2d26d0dad1dce5d494098fd667e6f4853a6`。
- `0036-experiment-v3-d2-static-stack.patch`: 固定frame版の閲覧用差分。SHA-256 `e1fb2d4c6f4ca7e9096e3f2399960de429b0bb0fea63cf35c2766082375ba1a8`。
- `0037-experiment-v3-two-function-lifetime.patch`: D3二関数版の閲覧用差分。SHA-256 `baeba03bffbd99d394042391216446d63432a44e5eb009534d0ebb2e2052f7c2`。

薄いv3 bundleは基準commitを持つrepository内で`git bundle verify`できる。通常はcommit/tree/clean状態を検査する`../scripts/prepare_sources.sh`を使う。

`experimental/`の差分はサイドチャネル原因の計測と固定work指数演算の試作であり、既定の低メモリ実装にも耐性claimにも含めない。
