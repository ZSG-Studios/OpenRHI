"""Prepare unmodified, version-pinned source and compiler distributions for Xmake."""
from pathlib import Path,PurePosixPath
import argparse,hashlib,json,os,platform,shutil,subprocess,tarfile,urllib.request,zipfile,uuid
from build_lock import file_lock
ROOT=Path(__file__).resolve().parents[2]
def sha(path):
    with Path(path).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
def _require(name,root=ROOT):
    root=Path(root).resolve();record=json.loads((root/'Xmake/dependencies.json').read_text())['dependencies'][name]
    base=Path(os.environ.get('OPENRHI_DEPS_ROOT',str(root/'Build/Dependencies'))).resolve()
    destination=base/name
    if (destination/'.openrhi-source.json').is_file():
        observed=json.loads((destination/'.openrhi-source.json').read_text())
        for key,value in record.items():
            actual=observed.get('observed_sha256') if key=='sha256' and 'observed_sha256' in observed else observed.get(key)
            if actual!=value:raise RuntimeError('Dependency cache pin changed; preserve and reprepare '+name)
        return destination
    base.mkdir(parents=True,exist_ok=True)
    if 'repository' in record:
        if record.get('sparse_paths'):
            if not (destination/'.git').is_dir():
                destination.mkdir(parents=True,exist_ok=True)
                subprocess.run(['git','init',str(destination)],check=True)
                subprocess.run(['git','-C',str(destination),'remote','add','origin',record['repository']],check=True)
            subprocess.run(['git','-C',str(destination),'fetch','--depth','1','--filter=blob:none','origin',record['revision']],check=True)
            subprocess.run(['git','-C',str(destination),'sparse-checkout','set','--no-cone',*record['sparse_paths']],check=True)
            subprocess.run(['git','-C',str(destination),'checkout','--detach',record['revision']],check=True)
        else:
            if not destination.exists():subprocess.run(['git','clone','--no-checkout',record['repository'],str(destination)],check=True)
            subprocess.run(['git','-C',str(destination),'checkout','--detach',record['revision']],check=True)
            subprocess.run(['git','-C',str(destination),'lfs','pull'],check=True)
        observed=subprocess.check_output(['git','-C',str(destination),'rev-parse','HEAD'],text=True).strip()
        if observed!=record['revision']:raise RuntimeError('Pinned Git revision mismatch: '+name)
        (destination/'.openrhi-source.json').write_text(json.dumps(record,indent=2)+'\n');return destination
    suffix='.tar.gz' if record['url'].endswith('.tar.gz') else '.zip'
    archive=base/(name+suffix)
    if archive.exists() and record.get('sha256') and sha(archive)!=record['sha256']:
        archive.rename(archive.with_name(archive.name+'.rejected-'+uuid.uuid4().hex))
    if not archive.exists():
        temporary=archive.with_name(archive.name+'.download-'+uuid.uuid4().hex)
        with urllib.request.urlopen(record['url'],timeout=180) as source,temporary.open('wb') as target:shutil.copyfileobj(source,target)
        if record.get('sha256') and sha(temporary)!=record['sha256']:raise RuntimeError('Archive SHA-256 mismatch: '+name)
        os.replace(temporary,archive)
    digest=sha(archive)
    published=destination
    if destination.exists():destination.rename(base/(name+'.incomplete-'+uuid.uuid4().hex))
    destination=base/(name+'.staging-'+uuid.uuid4().hex)
    destination.mkdir(parents=True,exist_ok=False)
    if suffix=='.zip':
        with zipfile.ZipFile(archive) as source:
            members=source.infolist();names=[m.filename for m in members if not m.is_dir()]
            tops={PurePosixPath(n).parts[0] for n in names}
            strip=next(iter(tops)) if len(tops)==1 and all(len(PurePosixPath(n).parts)>1 for n in names) else None
            for member in members:
                relative=PurePosixPath(member.filename)
                if strip:relative=PurePosixPath(*relative.parts[1:])
                target=(destination/str(relative)).resolve()
                if not target.is_relative_to(destination):raise RuntimeError('Unsafe archive path: '+member.filename)
                if member.is_dir():target.mkdir(parents=True,exist_ok=True)
                else:
                    target.parent.mkdir(parents=True,exist_ok=True)
                    with source.open(member) as a,target.open('wb') as b:shutil.copyfileobj(a,b)
    else:
        with tarfile.open(archive) as source:
            members=source.getmembers();tops={PurePosixPath(m.name).parts[0] for m in members}
            strip=next(iter(tops)) if len(tops)==1 else None
            for member in members:
                relative=PurePosixPath(member.name)
                if strip:relative=PurePosixPath(*relative.parts[1:])
                target=(destination/str(relative)).resolve()
                if not target.is_relative_to(destination):raise RuntimeError('Unsafe archive path: '+member.name)
                if member.isdir():target.mkdir(parents=True,exist_ok=True)
                elif member.isfile() or member.issym() or member.islnk():
                    target.parent.mkdir(parents=True,exist_ok=True)
                    payload=source.extractfile(member)
                    if payload:
                        with payload,target.open('wb') as output:shutil.copyfileobj(payload,output)
                        target.chmod(member.mode)
    (destination/'.openrhi-source.json').write_text(json.dumps(dict(record,observed_sha256=digest),indent=2)+'\n')
    destination.rename(published)
    return published
def require(name,root=ROOT):
    base=Path(os.environ.get('OPENRHI_DEPS_ROOT',str(Path(root).resolve()/'Build/Dependencies'))).resolve()
    with file_lock(base/(name+'.lock')):
        return _require(name,root)
def slang_host():
    host={'Windows':'windows','Darwin':'macos','Linux':'linux'}.get(platform.system())
    if not host:raise RuntimeError('Unsupported Slang host')
    arch='aarch64' if platform.machine().lower() in ('aarch64','arm64') else 'x86_64'
    return host+'-'+arch
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('dependencies',nargs='+');args=p.parse_args()
    for name in args.dependencies:print(name,require(name))
