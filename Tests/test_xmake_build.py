"""Regression checks for concurrent immutable Xmake dependency preparation."""
from pathlib import Path
import concurrent.futures,hashlib,json,os,subprocess,sys,tempfile,unittest,zipfile
ROOT=Path(__file__).resolve().parents[1]
class BuildInputs(unittest.TestCase):
    def test_parallel_cold_input_and_rejected_partial(self):
        retained=ROOT/'Build/DependencyConcurrency';retained.mkdir(parents=True,exist_ok=True)
        fixture=Path(tempfile.mkdtemp(prefix='cache-',dir=retained));(fixture/'Xmake').mkdir()
        archive=fixture/'source.zip'
        with zipfile.ZipFile(archive,'w') as stream:stream.writestr('source-1/input.h','immutable-header')
        digest=hashlib.sha256(archive.read_bytes()).hexdigest()
        (fixture/'Xmake/dependencies.json').write_text(json.dumps({'dependencies':{'fixture':{'url':archive.as_uri(),'sha256':digest}}}))
        cache=fixture/'cache';cache.mkdir();(cache/'fixture.zip').write_bytes(b'')
        code='import sys;from pathlib import Path;sys.path.insert(0,sys.argv[1]);from dependencies import require;print(require("fixture",Path(sys.argv[2])))'
        env=dict(os.environ,OPENRHI_DEPS_ROOT=str(cache))
        def run(_):return subprocess.run([sys.executable,'-c',code,str(ROOT/'Scripts/Build'),str(fixture)],env=env,capture_output=True,text=True)
        with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:results=list(pool.map(run,range(8)))
        self.assertTrue(all(p.returncode==0 for p in results),[(p.returncode,p.stderr) for p in results])
        self.assertEqual((cache/'fixture/input.h').read_text(),'immutable-header')
        self.assertEqual(len(list(cache.glob('fixture.zip.rejected-*'))),1)
        self.assertEqual(len(set(p.stdout for p in results)),1)
        self.assertEqual(hashlib.sha256((cache/'fixture.zip').read_bytes()).hexdigest(),digest)
        self.assertFalse(any(p.is_symlink() or p.is_junction() for p in cache.rglob('*')))
if __name__=='__main__':unittest.main()
