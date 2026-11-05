from pathlib import Path
import re
R=Path(__file__).resolve().parents[1];v='V9990';rows={}
for bank,p in [(0,R/'build/V9990/main.sym')]+[(int(re.search('art_s(\\d+)',p.name)[1]),p)for p in (R/'build/V9990').glob('art_s*_b3.sym')]:
 for area,n,a in re.findall(r'^\s*(\d+)\s+_(\w+)\s+([0-9A-F]{8})\s+[GL]*R\s*$',p.read_text(),re.M):
  if (bank==0 and area!='0')or(bank and area!='7'):continue
  rows[(bank,int(a,16)+(0x4000 if not bank else 0xA000))]=n
out=[]
for b in sorted({b for b,a in rows}):out.append('set ::ranges(%d) {%s}'%(b,' '.join('{%d %s}'%(a,n)for(bb,a),n in sorted(rows.items())if bb==b)))
(R/'tests/ranges_group012.tcl').write_text('\n'.join(out)+'\n')
t=(R/'tests/perf012.tcl').read_text().replace('set ::perf_times {};','set ::samples012 [dict create];set ::perf_times {};').replace(' set ::start [machine_info time];',' sample_group012\n set ::start [machine_info time];').replace(' puts stderr "CALLS $phase',' puts stderr "PROFILE $phase [lsort -decreasing -integer -index 0 [lmap {k v} $::samples012 {list $v $k}]]"\n puts stderr "CALLS $phase')
prefix='''source ranges_group012.tcl
proc sample_group012 {} {
 if {!$::perf_running} {return}
 set pc [reg PC];set bank [expr {$pc<40960?0:[debug read memory 0xC006]}];set key $bank:unknown
 if {[info exists ::ranges($bank)]} {foreach r $::ranges($bank) {lassign $r a n;if {$a>$pc} {break};set key $bank:$n}}
 dict incr ::samples012 $key;after time 0.0005 sample_group012
}
'''
# Tcl bundled by openMSX lacks lmap; use a sorting helper.
t=t.replace('[lsort -decreasing -integer -index 0 [lmap {k v} $::samples012 {list $v $k}]]','[sorted012]')
prefix+='proc sorted012 {} {set a {};dict for {k v} $::samples012 {lappend a [list $v $k]};return [lsort -decreasing -integer -index 0 $a]}\n'
(R/'tests/profile_group012.tcl').write_text(prefix+t)
