"""Acquire the pinned Xmake runtime without invoking another build system."""
from pathlib import Path
import argparse,hashlib,os,platform,shutil,subprocess,sys,urllib.request,zipfile,uuid
from build_lock import file_lock
VERSION='3.1.2'
URL='https://github.com/xmake-io/xmake/releases/download/v3.1.2/xmake-v3.1.2.win64.zip'
ARCHIVE_SHA256='be1c5fd5abec08ac14004daca7bfb1ec6e1f2f2dea94877de8f44dc6e0aed1b1'
EXE_SHA256='7339c480fbe44355f4ae63d954b741cbd7050aee7d3dd549e2feef40555c39bc'
def digest(path):
    with Path(path).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
def require_xmake(destination=None):
    override=os.environ.get('MIRA_XMAKE') or os.environ.get('OPENRHI_XMAKE')
    if override:
        executable=Path(override).resolve()
    elif platform.system()=='Windows':
        root=Path(destination).resolve() if destination else Path(__file__).resolve().parents[2]/'Build/Tools/Xmake'
        executable=root/'xmake/xmake.exe'
        with file_lock(root/'runtime.lock'):
            if not executable.is_file():
                root.mkdir(parents=True,exist_ok=True)
                archive=root/'xmake-3.1.2-win64.zip'
                if archive.exists() and digest(archive)!=ARCHIVE_SHA256:
                    archive.rename(archive.with_name(archive.name+'.rejected-'+uuid.uuid4().hex))
                if not archive.is_file():
                    temporary=archive.with_name(archive.name+'.download-'+uuid.uuid4().hex)
                    with urllib.request.urlopen(URL,timeout=120) as source,temporary.open('wb') as target:shutil.copyfileobj(source,target)
                    if digest(temporary)!=ARCHIVE_SHA256:raise RuntimeError('Pinned Xmake archive SHA-256 mismatch')
                    os.replace(temporary,archive)
                with zipfile.ZipFile(archive) as source:
                    for entry in source.infolist():
                        target=(root/entry.filename).resolve()
                        if not target.is_relative_to(root):raise RuntimeError('Unsafe Xmake archive path')
                    source.extractall(root)
        if digest(executable)!=EXE_SHA256:raise RuntimeError('Pinned Xmake executable SHA-256 mismatch')
    else:
        found=shutil.which('xmake')
        if not found:raise RuntimeError('Install Xmake 3.1.2 for this host or set OPENRHI_XMAKE')
        executable=Path(found)
    result=subprocess.run([str(executable),'--version'],capture_output=True,text=True,check=True)
    if not ('xmake v'+VERSION) in result.stdout:raise RuntimeError('Xmake 3.1.2 required: '+result.stdout.splitlines()[0])
    return executable
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--destination',type=Path)
    args=parser.parse_args();print(require_xmake(args.destination))
