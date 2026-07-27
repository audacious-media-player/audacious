#!/usr/bin/python
# Audacious installer for Windows 11 by nu11secur1ty 2026
import os
import shutil
import subprocess
import urllib.request
import zipfile
from pathlib import Path

# URLs and Target Paths
ZIP_URL = "https://distfiles.audacious-media-player.org/audacious-4.6.1-win32.zip"
WORK_DIR = Path(r"C:\Users\nu11secur1ty-tunnel\Desktop\Audacius")
WORK_DIR.mkdir(parents=True, exist_ok=True)

TEMP_ZIP = WORK_DIR / "audacious-4.6.1-win32.zip"
TEMP_EXTRACT = WORK_DIR / "temp_extract"
PROGRAM_FILES = Path(os.environ.get("ProgramFiles", r"C:\Program Files"))
INSTALL_DIR = PROGRAM_FILES / "Audacious"
DESKTOP_DIR = Path(os.environ.get("USERPROFILE")) / "Desktop"


def download_file(url, destination):
    """Download a file with progress indication."""
    print(f"[*] Downloading: {url}...")
    req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
    
    try:
        with urllib.request.urlopen(req) as response:
            total_size = int(response.headers.get('content-length', 0))
            block_size = 8192
            downloaded = 0
            
            with open(destination, "wb") as out_file:
                while True:
                    buffer = response.read(block_size)
                    if not buffer:
                        break
                    out_file.write(buffer)
                    downloaded += len(buffer)
                    if total_size > 0:
                        percent = (downloaded / total_size) * 100
                        print(f"\r  Progress: {percent:.1f}% ({downloaded // 1024} KB / {total_size // 1024} KB)", end="")
            
            print("\n[+] Download complete.")
            return True
    except Exception as e:
        print(f"\n[!] Error downloading: {e}")
        return False


def extract_zip(zip_path, extract_path):
    """Extract a ZIP file with progress indication."""
    print("[*] Extracting ZIP...")
    
    try:
        if extract_path.exists():
            shutil.rmtree(extract_path)
        
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            files = zip_ref.namelist()
            total_files = len(files)
            
            for idx, file in enumerate(files, 1):
                zip_ref.extract(file, extract_path)
                if idx % 100 == 0 or idx == total_files:
                    print(f"\r  Progress: {idx}/{total_files} files", end="")
        
        print("\n[+] Extraction complete.")
        return True
    except Exception as e:
        print(f"\n[!] Error extracting: {e}")
        return False


def copy_all_to_install_dir(extract_path, install_dir):
    """Copy all contents from extract_path to install_dir, handling nested directories."""
    
    # ПЪРВО създаваме директорията C:\Program Files\Audacious
    print(f"[*] Creating installation directory: {install_dir}...")
    install_dir.mkdir(parents=True, exist_ok=True)
    print(f"[+] Created directory: {install_dir}")
    
    # След това проверяваме дали има стара инсталация и я премахваме
    # (но вече директорията съществува, затова изтриваме само съдържанието)
    if any(install_dir.iterdir()):
        print(f"[*] Removing old installation contents...")
        for item in install_dir.iterdir():
            try:
                if item.is_dir():
                    shutil.rmtree(item)
                else:
                    item.unlink()
            except Exception as e:
                print(f"[!] Could not remove {item.name}: {e}")
                return False
    
    # Намираме какво има в извлечената папка
    extracted_items = list(extract_path.iterdir())
    
    # Проверяваме дали има една основна директория, която съдържа всичко
    single_dir = None
    if len(extracted_items) == 1 and extracted_items[0].is_dir():
        single_dir = extracted_items[0]
    
    if single_dir:
        # Копираме съдържанието от единичната директория
        print(f"[*] Copying from {single_dir.name} to {install_dir}...")
        source_path = single_dir
    else:
        # Копираме директно от извлечената папка
        print(f"[*] Copying directly to {install_dir}...")
        source_path = extract_path
    
    # Копираме всички елементи
    copied_count = 0
    for item in source_path.iterdir():
        dest_item = install_dir / item.name
        try:
            if item.is_dir():
                shutil.copytree(item, dest_item, dirs_exist_ok=True)
            else:
                shutil.copy2(item, dest_item)
            copied_count += 1
            print(f"\r  Copied: {copied_count} items", end="")
        except Exception as e:
            print(f"\n[!] Error copying {item.name}: {e}")
            return False
    
    print(f"\n[+] Copied {copied_count} items to {install_dir}")
    return True


def create_shortcut(target_exe, shortcut_path, working_dir=None):
    """Create a Windows shortcut using VBScript."""
    if not target_exe.exists():
        print(f"[!] Target executable not found: {target_exe}")
        return False
    
    if working_dir is None:
        working_dir = target_exe.parent
    
    vbs_script = f'''
    Set WshShell = CreateObject("WScript.Shell")
    Set shortcut = WshShell.CreateShortcut("{shortcut_path}")
    shortcut.TargetPath = "{target_exe}"
    shortcut.WorkingDirectory = "{working_dir}"
    shortcut.IconLocation = "{target_exe}, 0"
    shortcut.Description = "Audacious Media Player"
    shortcut.Save
    '''
    
    vbs_file = WORK_DIR / "create_shortcut.vbs"
    try:
        with open(vbs_file, "w", encoding="utf-8") as f:
            f.write(vbs_script)
        
        subprocess.run(
            ["cscript", "//Nologo", str(vbs_file)],
            check=True,
            capture_output=True,
            text=True
        )
        print(f"[+] Shortcut created on Desktop: {shortcut_path}")
        return True
    except subprocess.CalledProcessError as e:
        print(f"[!] Error creating shortcut: {e.stderr}")
        return False
    except Exception as e:
        print(f"[!] Unexpected error creating shortcut: {e}")
        return False
    finally:
        if vbs_file.exists():
            vbs_file.unlink()


def cleanup():
    """Clean up temporary files and directories."""
    print("[*] Cleaning up...")
    
    if TEMP_ZIP.exists():
        TEMP_ZIP.unlink()
        print("  - Removed ZIP file")
    
    if TEMP_EXTRACT.exists():
        shutil.rmtree(TEMP_EXTRACT)
        print("  - Removed temporary extraction directory")


def find_audacious_exe(install_dir):
    """Find audacious.exe in the installation directory."""
    possible_locations = [
        install_dir / "bin" / "audacious.exe",
        install_dir / "audacious.exe",
        install_dir / "Audacious" / "bin" / "audacious.exe",
        install_dir / "Audacious" / "audacious.exe",
    ]
    
    for loc in possible_locations:
        if loc.exists():
            return loc
    
    # Try to find it recursively (but not too deep)
    for root, dirs, files in os.walk(install_dir):
        if "audacious.exe" in files:
            return Path(root) / "audacious.exe"
        # Limit search depth
        if str(root).count(os.sep) - str(install_dir).count(os.sep) > 3:
            continue
    
    return None


def main():
    """Main execution function."""
    print("=" * 60)
    print("  Audacious Media Player - Automated Installer")
    print("=" * 60)
    
    # 1. Download the ZIP file
    if not download_file(ZIP_URL, TEMP_ZIP):
        return 1
    
    # 2. Extract Archive
    if not extract_zip(TEMP_ZIP, TEMP_EXTRACT):
        return 1
    
    # 3. Copy everything to Program Files
    # ТУК функцията ПЪРВО създава C:\Program Files\Audacious
    if not copy_all_to_install_dir(TEMP_EXTRACT, INSTALL_DIR):
        return 1
    
    # 4. Find audacious.exe
    target_exe = find_audacious_exe(INSTALL_DIR)
    if not target_exe:
        print("[!] Could not find audacious.exe in the installation directory")
        return 1
    
    print(f"[*] Found executable: {target_exe}")
    
    # 5. Create Desktop Shortcut
    shortcut_path = DESKTOP_DIR / "Audacious.lnk"
    if not create_shortcut(target_exe, shortcut_path):
        return 1
    
    # 6. Cleanup
    cleanup()
    
    print("\n" + "=" * 60)
    print("[✔] Installation finished successfully!")
    print(f"[*] Audacious installed to: {INSTALL_DIR}")
    print(f"[*] Shortcut created at: {shortcut_path}")
    print("=" * 60)
    
    return 0


if __name__ == "__main__":
    exit_code = main()
    exit(exit_code)
