"""Compile maintained qualification kernels through an Xmake-owned build hook."""
from pathlib import Path
import argparse,json,os
from shader import compile_shader,parser
ROOT=Path(__file__).resolve().parents[2]
def build(output,target,ui=False):
    output=Path(output);output.mkdir(parents=True,exist_ok=True)
    manifest=[('DescriptorCompute','DescriptorCompute.slang','descriptorMain','compute',()),('IndirectDraw.vs','IndirectDraw.slang','vsMain','vertex',()),('IndirectDraw.ps','IndirectDraw.slang','psMain','fragment',()),('MeshDraw','MeshDraw.slang','meshMain','mesh',()),('TaskDraw','MeshDraw.slang','taskMain','amplification',())]
    if target=='dxil':manifest.extend([('BindlessCompute','BindlessCompute.slang','descriptorMain','compute',()),('BindlessTextureSampler','BindlessTextureSampler.slang','textureSamplerMain','compute',()),('BindlessNonUniformStored','BindlessNonUniformIndex.slang','nonUniformIndexMain','compute',('OPENRHI_TEST_STORED_NONUNIFORM=1',)),('BindlessNonUniformDirect','BindlessNonUniformIndex.slang','nonUniformIndexMain','compute',('OPENRHI_TEST_STORED_NONUNIFORM=0',))])
    extension='metallib' if target=='metal' and os.sys.platform=='darwin' else target
    for name,source,entry,stage,defines in manifest:
        arguments=['--source',str(ROOT/'Tests'/source),'--target',target,'--entry',entry,'--stage',stage,'--include',str(ROOT/'Include'),'--output',str(output/(name+'.'+extension))]
        for item in defines:arguments+=['--define',item]
        compile_shader(parser().parse_args(arguments))
    if ui:
        for output_target in ('spirv','metal'):
            for suffix,entry,stage in (('vs','RmlUiVS','vertex'),('fs','RmlUiFS','fragment')):
                name='RmlUiContract_'+output_target+'_'+suffix
                compile_shader(parser().parse_args(['--source',str(ROOT/'Shaders/RmlUi.slang'),'--target',output_target,'--entry',entry,'--stage',stage,'--include',str(ROOT/'Include'),'--output',str(output/(name+'.'+output_target))]))
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True);p.add_argument('--target',required=True);p.add_argument('--ui',action='store_true');args=p.parse_args();build(args.output,args.target,args.ui)
