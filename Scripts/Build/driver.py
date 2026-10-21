"""Run the OpenRHI Xmake build, tests and SDK tasks from an isolated owner root."""
from pathlib import Path
import argparse,subprocess,sys
from bootstrap import require_xmake
ROOT=Path(__file__).resolve().parents[2]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('action',choices=('configure','build','test','sdk','check'))
    p.add_argument('--mode',choices=('debug','release'),default='release');p.add_argument('--all-configurations',action='store_true')
    p.add_argument('--output',default='_OpenRHI_SDK');p.add_argument('--with-compiler',action='store_true')
    args,forwarded=p.parse_known_args();tool=str(require_xmake())
    def run(command):subprocess.run([tool,*command,'-P',str(ROOT)],cwd=ROOT,check=True)
    if args.action=='configure':run(['f','-m',args.mode,'-y',*forwarded])
    elif args.action=='build':
        for mode in (('debug','release') if args.all_configurations else (args.mode,)):
            run(['f','-m',mode,'-y']);run(['build','-y',*(forwarded or ['OpenRHI'])])
    elif args.action=='test':run(['test',*forwarded])
    elif args.action=='sdk':run(['sdk','--output='+str((ROOT/args.output).resolve()),*(['--with-compiler'] if args.with_compiler else []),*forwarded])
    else:run(['openrhi-check',*forwarded])
if __name__=='__main__':main()
