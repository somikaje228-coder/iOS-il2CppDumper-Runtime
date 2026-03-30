# iOS Il2CppDumper Runtime (In-Game)
### 🎮 Specialized for Unity Engine Games (ARM64)

![iOS Support](https://img.shields.io/badge/Platform-iOS-lightgrey.svg)
![Type](https://img.shields.io/badge/Type-Runtime_Dumper-blue.svg)
![Engine](https://img.shields.io/badge/Engine-Unity_IL2CPP-green.svg)

Đây là phiên bản **Il2CppDumper** được tối ưu hóa đặc biệt để chạy trực tiếp bên trong các trò chơi phát triển trên nền tảng **Unity Engine (IL2CPP)**. Thay vì trích xuất thủ công trên PC, công cụ này sẽ thực hiện Dump dữ liệu ngay từ bộ nhớ khi game đang vận hành, giúp tối ưu hóa quy trình Reverse Engineering trên môi trường iOS.

---

## ✨ Tính năng nổi bật

* **Unity Engine Focus:** Chuyên dụng để giải mã cấu trúc các trò chơi sử dụng công nghệ IL2CPP.
* **Memory Runtime Dumping:** Kết xuất dữ liệu trực tiếp từ RAM, vượt qua một số lớp bảo vệ file tĩnh.
* **Smart Auto-Detection:** Tự động nhận diện `UnityFramework` hoặc Binary chính để bắt đầu quá trình.
* **Arm64 Optimization:** Tương thích hoàn hảo với kiến trúc chip Apple Silicon mới nhất.
* **Auto-Export to Documents:** Xuất kết quả gọn gàng vào thư mục `Documents` của ứng dụng.
    > 📍 **Path:** `/var/mobile/Containers/Data/Application/<UUID>/Documents/`

---

## 📂 Kết quả đầu ra (Output)

Sau khi hoàn tất, toàn bộ dữ liệu sẽ nằm trong thư mục:  
`Documents/<main_binary>_UNITYDUMP/`

Danh sách tệp tin:
* **`dump.cs`**: Chứa toàn bộ Class, Field, Method và Offset của Game Unity.
* **`ida.py`**: Script hỗ trợ nạp Symbol nhanh cho IDA Pro.
* **`il2cpp.h`**: File Header định nghĩa các Struct phục vụ phân tích chuyên sâu.

---

## ⚠️ Lưu ý quan trọng (Disclaimer)

Công cụ này chỉ hỗ trợ các tựa game **Unity Engine (IL2CPP)**. Tuy nhiên:

> [!CAUTION]
> **Biện pháp chống ngược dòng (Anti-Reverse Engineering):**
> Công cụ sẽ **KHÔNG HOẠT ĐỘNG** (hoặc trả về dữ liệu rỗng) đối với các trò chơi đã thực hiện **Symbol Stripping** (ẩn bảng ký hiệu). Trong những trường hợp này, các hàm sẽ không thể được ánh xạ (map) chính xác về tên gốc.

---

## 🚀 Cách sử dụng

1.  **Tích hợp:** Inject dylib vào file `.ipa` hoặc cài đặt file `.deb` (cho máy Jailbreak).
2.  **Khởi chạy:** Mở trò chơi Unity trên thiết bị iOS.
3.  **Thực thi:** Quá trình Dump sẽ tự động kích hoạt sau vài giây khi Game đã load xong tài nguyên vào bộ nhớ.
4.  **Truy xuất:** Sử dụng **Filza File Manager** để truy cập thư mục `Documents` của Game và lấy kết quả.

---

## 🙏 Credits & Thanks

Dự án này được hoàn thiện nhờ sự đóng góp và kế thừa từ các nhà phát triển:

* **Mr D - DS Gaming** – Phát triển & tối ưu hóa phiên bản Runtime cho iOS.
* **[Tien0246](https://github.com/tien0246)** & **[Batchhh](https://github.com/Batchhh)** – Phát triển phiên bản tiền nhiệm cho iOS Il2CppDumper Runtime.
* **[Perfare (Il2CppDumper)](https://github.com/Perfare/Il2CppDumper)** – Tác giả gốc của giải pháp Il2CppDumper.
* **[Zygisk-Il2CppDumper](https://github.com/Perfare/Zygisk-Il2CppDumper)** – Cảm hứng về cơ chế Hooking hiện đại.