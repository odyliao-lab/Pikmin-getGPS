# Pikmin GPS Copy

[繁體中文](#繁體中文) · [English](#english)

An unofficial Magisk/Zygisk module that copies the GPS coordinates of the
selected **Pikmin Bloom expedition-list item** to the Android clipboard.

> [!IMPORTANT]
> This project is unofficial and is not affiliated with, endorsed by, or
> sponsored by Niantic, Nintendo, or The Pokémon Company. Rooting a device and
> modifying a game process can affect device security, game stability, and
> account eligibility. You use this module at your own risk.

## 繁體中文

### 功能

在 Pikmin Bloom 的「探險」清單點選道具時，本模組會：

1. 讀取**剛剛點選的那一筆探險道具**的 `SpawnLocation`。
2. 將座標以 `緯度,經度` 格式複製到 Android 剪貼簿，小數保留七位。
3. 顯示「GPS 已複製」提示。

例如：`25.0000000,121.5000000`

模組不會連線到任何額外伺服器，也不會開始、取消或修改探險。

### 系統需求與相容性

| 項目 | 需求 |
| --- | --- |
| 手機 | 已 root 的 Android 手機 |
| Root 管理 | Magisk 24.0 以上，並啟用內建 Zygisk；實機測試版本為 Magisk 30.7 |
| CPU | `arm64-v8a`（64 位元 ARM） |
| Android | Pikmin Bloom 本身支援的 Android 版本；模組原生程式以 Android API 28 以上為目標 |
| 遊戲 | **Pikmin Bloom 151.0**，versionCode **1786062771** |
| 安裝檔 | [`dist/pikmin-gps-copy-v151-r11.zip`](dist/pikmin-gps-copy-v151-r11.zip) |

一般安裝與使用**不需要**電腦、ADB、LSPosed 或 Frida。

> [!WARNING]
> 本版本只接受上表的精確遊戲版本。Pikmin Bloom 更新後，模組會因版本
> 簽章不符而拒絕掛鉤，不會猜測新位置；請等待相容版本。

### 安裝

1. 下載 [`pikmin-gps-copy-v151-r11.zip`](dist/pikmin-gps-copy-v151-r11.zip)。
2. 可選：比對檔案 SHA-256：

   ```text
   DEBA6317E0F2D52447263AB0561A92C689C64309219F8070952435624CA2D995
   ```

3. 開啟 Magisk →「設定」，確認 **Zygisk** 已啟用。
4. 如果有啟用「強制執行 DenyList」，請確認 Pikmin Bloom 不在 DenyList 中。
5. 開啟 Magisk →「模組」→「從本機安裝」，選擇下載的 ZIP。
6. 安裝完成後重新啟動手機。
7. 開啟 Pikmin Bloom，進入「探險」清單，點選一個道具。看到「GPS
   已複製」後，即可在其他 App 貼上座標。

若剛重新開機後第一次沒有顯示提示，請將 Pikmin Bloom 從最近使用的
App 中完整關閉，再重新開啟一次。

### 使用範圍與限制

- 支援「探險」**清單項目**中的禮物盒、花苗、水果等道具。
- 目前不支援探險畫面上方地圖標記、蘑菇、道具詳情中的左右輪播。
- 每次點選成功都會覆寫 Android 剪貼簿目前的內容。
- 僅提供 `arm64-v8a`，不支援 32 位元 ARM 或 x86 裝置。
- 遊戲更新、伺服器端行為或 UI 流程變動，都可能使功能失效。
- 本模組只擷取遊戲當下已載入之探險項目座標，不提供背景掃描。

### 隱私與安全

- 模組本身沒有網路上傳功能。
- 公開安裝包**不會開啟 ADB、無線 ADB 或任何遠端連線服務**。
- 座標只會寫入本機 Android 剪貼簿；其他具有剪貼簿讀取權限的 App
  可能看得到其內容。
- 建議安裝前備份重要資料，並確認你了解 root 與 Zygisk 模組的風險。

### 移除

在 Magisk 的「模組」頁面停用或移除 **Pikmin GPS Copy**，然後重新
啟動手機。

### 從原始碼建置（Windows）

建置時需要：

- Windows PowerShell 5.1 或 PowerShell 7
- Android NDK r27d
- CMake 3.18.1 以上
- Ninja
- Windows 內建 `tar.exe`（用於建立 Magisk ZIP）

不使用預設 NDK 路徑時，請明確指定：

```powershell
.\build.ps1 -NdkPath 'C:\path\to\android-ndk-r27d'
.\package.ps1
```

產物：

- 原生模組：`build\zygisk\arm64-v8a.so`
- Magisk 安裝包：`dist\pikmin-gps-copy-v151-r11.zip`

ADB/Android SDK 只在實機除錯時需要，不是建置或安裝的必要元件。

### 專案結構與第三方元件

- `cpp/`：Zygisk 模組與 GPS 擷取邏輯
- `template/magisk_module/`：Magisk 安裝包範本
- `build.ps1`：ARM64 原生程式建置
- `package.ps1`：產生並檢查公開版 ZIP
- [`Zygisk API`](https://github.com/topjohnwu/zygisk-module-sample)：
  官方範例 API 標頭（0BSD）
- [`And64InlineHook`](https://github.com/Rprop/And64InlineHook)：MIT
- [`xDL`](https://github.com/hexhacking/xDL)：Android 動態連結器輔助程式（MIT）
- 完整第三方授權文字請見 [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)。

## English

### What it does

When you tap an item in the Pikmin Bloom Expeditions list, this module:

1. Reads the `SpawnLocation` of the **exact list item you selected**.
2. Copies the coordinates to the Android clipboard as
   `latitude,longitude`, with seven decimal places.
3. Shows a “GPS 已複製” (“GPS copied”) toast.

Example: `25.0000000,121.5000000`

The module does not contact an additional server, and it does not start,
cancel, or modify an expedition.

### Requirements and compatibility

| Component | Requirement |
| --- | --- |
| Device | Rooted Android phone |
| Root manager | Magisk 24.0 or later with built-in Zygisk enabled; tested on Magisk 30.7 |
| CPU | `arm64-v8a` (64-bit ARM) |
| Android | A version supported by Pikmin Bloom; the native module targets Android API 28+ |
| Game | **Pikmin Bloom 151.0**, versionCode **1786062771** |
| Package | [`dist/pikmin-gps-copy-v151-r11.zip`](dist/pikmin-gps-copy-v151-r11.zip) |

Normal installation and use require **no** computer, ADB, LSPosed, or Frida.

> [!WARNING]
> This release accepts only the exact game version listed above. After Pikmin
> Bloom is updated, the module fails closed on a signature mismatch instead of
> guessing new offsets. Wait for a compatible module release.

### Installation

1. Download [`pikmin-gps-copy-v151-r11.zip`](dist/pikmin-gps-copy-v151-r11.zip).
2. Optional: verify the file's SHA-256:

   ```text
   DEBA6317E0F2D52447263AB0561A92C689C64309219F8070952435624CA2D995
   ```

3. Open Magisk → Settings and make sure **Zygisk** is enabled.
4. If “Enforce DenyList” is enabled, make sure Pikmin Bloom is not on the
   DenyList.
5. Open Magisk → Modules → Install from storage, then select the downloaded ZIP.
6. Reboot the phone when installation finishes.
7. Open Pikmin Bloom, enter the Expeditions list, and tap an item. After the
   “GPS 已複製” toast appears, paste the coordinates into another app.

If no toast appears on the first attempt immediately after a reboot, fully
close Pikmin Bloom from the recent-apps screen and launch it once more.

### Scope and limitations

- Supports gifts, seedlings, fruit, and similar **Expeditions list items**.
- Does not currently support markers on the map at the top of the Expeditions
  screen, mushrooms, or horizontal item browsing in the detail view.
- Each successful tap replaces the current Android clipboard contents.
- Only `arm64-v8a` is provided; 32-bit ARM and x86 devices are unsupported.
- Game updates, server-side behavior, or UI-flow changes may break the module.
- It reads expedition items already loaded by the game; it is not a background
  scanner.

### Privacy and safety

- The module contains no network-upload feature.
- The public package **does not enable ADB, wireless ADB, or any remote-access
  service**.
- Coordinates are written only to the local Android clipboard. Other apps with
  clipboard access may be able to read them.
- Back up important data and understand the risks of root and Zygisk modules
  before installation.

### Uninstall

Disable or remove **Pikmin GPS Copy** on Magisk's Modules screen, then reboot.

### Building from source on Windows

Build requirements:

- Windows PowerShell 5.1 or PowerShell 7
- Android NDK r27d
- CMake 3.18.1 or later
- Ninja
- Windows `tar.exe` for creating the Magisk ZIP

If the NDK is not at the script's default path, specify it explicitly:

```powershell
.\build.ps1 -NdkPath 'C:\path\to\android-ndk-r27d'
.\package.ps1
```

Outputs:

- Native module: `build\zygisk\arm64-v8a.so`
- Magisk package: `dist\pikmin-gps-copy-v151-r11.zip`

ADB/the Android SDK is needed only for on-device debugging, not for building
or installing the module.

### Project layout and third-party components

- `cpp/`: Zygisk module and GPS capture logic
- `template/magisk_module/`: Magisk installer template
- `build.ps1`: builds the ARM64 native module
- `package.ps1`: creates and validates the public ZIP
- [`Zygisk API`](https://github.com/topjohnwu/zygisk-module-sample):
  official sample API header (0BSD)
- [`And64InlineHook`](https://github.com/Rprop/And64InlineHook): MIT
- [`xDL`](https://github.com/hexhacking/xDL): Android dynamic-linker helper
  (MIT)
- See [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) for the full
  third-party license texts.
