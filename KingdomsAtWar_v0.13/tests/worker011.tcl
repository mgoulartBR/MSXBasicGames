source helpers011.tcl
proc worker_case {} {
 delay 65;materials;fund 96 48
 ck harvested_start {[w wood]>=64 && [w gold]>=32 && [conserved]}
 choose 0;camera_until cam_y 0 32;point 120 204;tap 5 128;world_point 0 10
 ck forge_locked_without_quartel {[b build_kind]==4 && ![b place_valid] && [conserved]};tap 7 4
 set house [build 1 0 0]
 choose_building 0;tap 4 128
 ck worker_reserved {[bd 0 training]==1 && [conserved]}
 tap 3 1;ck worker_cancel_refund {[bd 0 training]==0 && [b unit_count]==4 && [conserved]}
 tap 4 128;await {[b unit_count]==5} 30
 ck fifth_is_worker {[b unit_kind 4]==0 && [conserved]}
 set saw [build 3 4 14]
 choose 4;world_point 10 13;tap 5 32
 set bank [w wood];await {[w wood]>$bank} 40
 ck new_worker_delivers_to_sawmill {[b deposit_id 4]==$saw && [w deposits]>0 && [conserved]}
 await {[b stock [expr {13*32+10}]]==0 && [b cargo 4]==0 && [b worker_task 4]==0} 60
 choose 4;world_point 9 7;tap 5 32
 await {[b mine_phase 4]==2 && [b actor_visible 4]==0} 60
 ck recruited_worker_hidden_in_mine {[b actor_visible 4]==0 && [b worker_task 4]==2 && [conserved]}
 shot new_worker_in_mine
 world_point 7 5;tap 5 32
 await {[b mine_phase 4]==0 && [unit 4 state]==3} 40
 ck miner_can_exit_on_order {[b worker_task 4]==0 && [conserved]}
 move_unit 3 11 5
 choose 4;camera_until cam_y 0 32;point 56 204;tap 5 128;world_point 8 0;tap 5 128
 set id [b unit_build 4]
 await {([b builder_working]&16)!=0 && [bd $id remaining]<180} 40
 ck recruited_worker_builds {[bd $id state]==1 && [bd $id remaining]<180 && [conserved]}
 point 127 8;tap 5 32;set remaining [bd $id remaining];delay 1
 ck pause_preserves_progress {[bd $id state]==1 && [bd $id worker]==255 && [bd $id remaining]==$remaining}
 world_point 8 0;tap 5 32
 await {[bd $id state]==2 || (([b builder_working]&16)!=0 && [bd $id remaining]<$remaining)} 40
 ck resume_preserves_cost {([bd $id state]==2 || [bd $id remaining]<$remaining) && [conserved]}
 # Separate fresh foundation for cancellation, before the worker can finish it.
 choose 4;point 24 204;tap 5 128;world_point 4 3;tap 5 128
 set cancel_id [b unit_build 4]
 ck fresh_cancel_site {$cancel_id!=255 && [bd $cancel_id state]==1}
 tap 3 1;ck cancel_site_refund {[bd $cancel_id state]==0 && [b unit_build 4]==255 && [conserved]}
 ck no_hud_erases {[w hud_erase_count]==0}
 shot worker07
 puts stderr "RESULT failures=$::failures";exit
}
coroutine worker07 guard worker_case
after realtime 360 {puts stderr "FAIL worker watchdog";exit}
