// launcher.cpp
// Portable launcher for JetBrains IDEs (IntelliJ/PyCharm) with LightEdit,
// portable properties/vmoptions, YAML config, and SHA-256 verification.

#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <locale>
#include <codecvt>
#include <filesystem>

static std::wstring ws_trim(const std::wstring &s) {
    size_t a = 0, b = s.size();
    while (a < b && iswspace(s[a])) ++a;
    while (b > a && iswspace(s[b - 1])) --b;
    return s.substr(a, b - a);
}

static std::wstring ws_lower(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return towlower(c); });
    return s;
}

static bool starts_with(const std::wstring &s, const std::wstring &prefix) {
    if (s.size() < prefix.size()) return false;
    return std::equal(prefix.begin(), prefix.end(), s.begin());
}

static std::wstring quote(const std::wstring &s) {
    if (s.find_first_of(L" \t\"") == std::wstring::npos) return s;
    std::wstring out = L"\"";
    for (wchar_t c: s) {
        if (c == L'"') out += L'\\';
        out += c;
    }
    out += L"\"";
    return out;
}

// Very small "YAML-like" parser: supports lines "key: value" or "key=value".
// '#' and ';' start comments. No nesting. Keys are case-insensitive.
static std::map<std::wstring, std::wstring> parse_kv_file(const std::wstring &path) {
    std::map<std::wstring, std::wstring> kv;
    std::wifstream f{std::filesystem::path(path)};
    f.imbue(std::locale(f.getloc(), new std::codecvt_utf8<wchar_t>));
    if (!f) return kv;
    std::wstring line;
    while (std::getline(f, line)) {
        auto hash = line.find(L'#');
        if (hash != std::wstring::npos) line = line.substr(0, hash);
        auto sc = line.find(L';');
        if (sc != std::wstring::npos) line = line.substr(0, sc);
        line = ws_trim(line);
        if (line.empty()) continue;
        size_t pos = line.find(L':');
        if (pos == std::wstring::npos) pos = line.find(L'=');
        if (pos == std::wstring::npos) continue;
        std::wstring k = ws_trim(line.substr(0, pos));
        std::wstring v = ws_trim(line.substr(pos + 1));
        kv[ws_lower(k)] = v;
    }
    return kv;
}

// Hex helpers
static std::wstring to_hex(const std::vector<BYTE> &data) {
    static const wchar_t *HEX = L"0123456789abcdef";
    std::wstring out;
    out.resize(data.size() * 2);
    for (size_t i = 0; i < data.size(); ++i) {
        out[2 * i] = HEX[(data[i] >> 4) & 0xF];
        out[2 * i + 1] = HEX[data[i] & 0xF];
    }
    return out;
}

static std::vector<BYTE> from_hex(const std::wstring &hex) {
    auto h = ws_lower(hex);
    std::vector<BYTE> out;
    out.reserve(h.size() / 2);
    auto val = [](wchar_t c)-> int {
        if (c >= L'0' && c <= L'9') return c - L'0';
        if (c >= L'a' && c <= L'f') return 10 + (c - L'a');
        return -1;
    };
    for (size_t i = 0; i + 1 < h.size(); i += 2) {
        int hi = val(h[i]);
        int lo = val(h[i + 1]);
        if (hi < 0 || lo < 0) return {};
        out.push_back((BYTE) ((hi << 4) | lo));
    }
    return out;
}

// SHA-256 using BCrypt
static bool sha256_file(const std::wstring &path, std::wstring &hexOut) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;

    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    NTSTATUS s = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (s != 0) {
        CloseHandle(h);
        return false;
    }

    DWORD objLen = 0, cbData = 0;
    s = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (PUCHAR) &objLen, sizeof(objLen), &cbData, 0);
    if (s != 0) {
        BCryptCloseAlgorithmProvider(alg, 0);
        CloseHandle(h);
        return false;
    }
    std::vector<BYTE> obj(objLen);

    s = BCryptCreateHash(alg, &hash, obj.data(), objLen, nullptr, 0, 0);
    if (s != 0) {
        BCryptCloseAlgorithmProvider(alg, 0);
        CloseHandle(h);
        return false;
    }

    const DWORD BUFSZ = 1 << 16;
    std::vector<BYTE> buf(BUFSZ);
    DWORD read = 0;
    while (ReadFile(h, buf.data(), BUFSZ, &read, nullptr) && read > 0) {
        s = BCryptHashData(hash, buf.data(), read, 0);
        if (s != 0) {
            BCryptDestroyHash(hash);
            BCryptCloseAlgorithmProvider(alg, 0);
            CloseHandle(h);
            return false;
        }
    }
    CloseHandle(h);

    DWORD hashLen = 0;
    s = BCryptGetProperty(alg, BCRYPT_HASH_LENGTH, (PUCHAR) &hashLen, sizeof(hashLen), &cbData, 0);
    if (s != 0) {
        BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(alg, 0);
        return false;
    }
    std::vector<BYTE> digest(hashLen);

    s = BCryptFinishHash(hash, digest.data(), hashLen, 0);
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(alg, 0);
    if (s != 0) return false;

    hexOut = to_hex(digest);
    return true;
}

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR lpCmdLine, int) {
    // Base dir of launcher.exe
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring base_directory(exePath);
    size_t pos = base_directory.find_last_of(L"\\/");
    if (pos != std::wstring::npos) base_directory = base_directory.substr(0, pos);

    // Paths
    // Config YAML now resides in the data directory
    std::wstring cfgYaml = base_directory + L"\\data\\launcher.yaml";

    auto kv = parse_kv_file(cfgYaml);

    // Defaults
    std::wstring exeName = kv.count(L"exe_name") ? kv[L"exe_name"] : L"idea64.exe";
    // original_vmoptions_filename is implicit: always exe_name + ".vmoptions"
    std::wstring original_vmoptions_filename = exeName + L".vmoptions";
    // Properties filename is stable across JetBrains products
    std::wstring original_properties_filename = L"idea.properties";
    std::wstring portable_vmoptions_filename = kv.count(L"portable_vmoptions_name")
                                                   ? kv[L"portable_vmoptions_name"]
                                                   : L"idea.vmoptions";
    std::wstring portable_properties_filename = kv.count(L"portable_properties_name")
                                                    ? kv[L"portable_properties_name"]
                                                    : L"idea.properties";
    std::wstring original_vmoptions_sha = kv.count(L"original_vmoptions_sha")
                                              ? ws_lower(kv[L"original_vmoptions_sha"])
                                              : L"";
    std::wstring original_properties_sha = kv.count(L"original_properties_sha")
                                               ? ws_lower(kv[L"original_properties_sha"])
                                               : L"";

    bool light_edit = true;
    if (kv.count(L"light_edit")) {
        auto v = ws_lower(kv[L"light_edit"]);
        light_edit = (v == L"1" || v == L"true" || v == L"yes");
    }

    // Build key paths
    std::wstring bin_directory = base_directory + L"\\app\\bin";
    std::wstring data_directory = base_directory + L"\\data";

    std::wstring exe_full = bin_directory + L"\\" + exeName;
    std::wstring original_vmoptions = bin_directory + L"\\" + original_vmoptions_filename;
    std::wstring original_properties = bin_directory + L"\\" + original_properties_filename;

    std::wstring portable_vmoptions = data_directory + L"\\" + portable_vmoptions_filename;
    std::wstring portable_properties = data_directory + L"\\" + portable_properties_filename;

    // Verify SHA-256 of original files if hashes provided
    bool any_mismatch = false;

    if (!original_vmoptions_sha.empty()) {
        std::wstring got;
        if (sha256_file(original_vmoptions, got)) {
            if (ws_lower(got) != ws_lower(original_vmoptions_sha)) {
                any_mismatch = true;
            }
        } else {
            MessageBoxW(nullptr, (L"Cannot read: " + original_vmoptions).c_str(), L"VM Options missing", MB_ICONWARNING | MB_OK);
        }
    }
    if (!original_properties_sha.empty()) {
        std::wstring got;
        if (sha256_file(original_properties, got)) {
            if (ws_lower(got) != ws_lower(original_properties_sha)) {
                any_mismatch = true;
            }
        } else {
            MessageBoxW(nullptr, (L"Cannot read: " + original_properties).c_str(), L"Properties missing",
                        MB_ICONWARNING | MB_OK);
        }
    }

    if (any_mismatch) {
        std::wstring msg = L"original files:\n- " + original_vmoptions_filename + L" or\n- " + original_properties_filename +
                           L"\nwas changed\n\n- review changes\n- review/edit data/idea.properties and data/idea.vmoptions\n- execute update-hash.ps1 to generate new sha\n- update data/launcher.yaml as administrator";
        MessageBoxW(nullptr, msg.c_str(), L"Checksum mismatch", MB_ICONWARNING | MB_OK);
    }

    // Derive environment variable prefix from exe_name: strip .exe, strip trailing digits, uppercase
    auto derive_prefix = [](std::wstring name) {
        // remove extension
        if (name.size() >= 4) {
            auto low = ws_lower(name);
            if (low.rfind(L".exe") == low.size() - 4) name = name.substr(0, name.size() - 4);
        }
        // remove trailing digits
        while (!name.empty() && iswdigit(name.back())) name.pop_back();
        // uppercase
        std::transform(name.begin(), name.end(), name.begin(), [](wchar_t c) { return towupper(c); });
        return name;
    };
    std::wstring prefix = derive_prefix(exeName);

    // Build env var names
    std::wstring environment_properties_name = prefix + L"_PROPERTIES";
    std::wstring environment_vmoptions_name = prefix + L"_VM_OPTIONS";

    // Set environment for child process (portable)
    SetEnvironmentVariableW(environment_properties_name.c_str(), portable_properties.c_str());
    SetEnvironmentVariableW(environment_vmoptions_name.c_str(), portable_vmoptions.c_str());

    // Build command line
    std::wstring cmd = quote(exe_full);
    if (light_edit) cmd += L" -e";
    if (lpCmdLine && *lpCmdLine) {
        cmd += L" ";
        cmd += lpCmdLine; // already a raw string; quoting left to shell (we're using CreateProcess with full cmdline)
    }

    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    BOOL ok = CreateProcessW(
        /*app*/ nullptr,
                /*cmd*/ cmd.data(),
                nullptr, nullptr, FALSE,
                0, nullptr, base_directory.c_str(),
                &si, &pi
    );

    if (ok) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return 0;
    } else {
        std::wstring msg = L"Failed to start: " + exe_full + L"\nCheck exe_name in launcher.yaml.";
        MessageBoxW(nullptr, msg.c_str(), L"Launcher Error", MB_ICONERROR | MB_OK);
        return 1;
    }
}
