#define main sim_main
#include "tools/livescan/sim_live.cpp"
#undef main
#include <map>
static std::map<int, std::vector<uint8_t>> sd, nvs; static bool card = true; static bool sdwrite_ok = true;
static int ld(uint8_t raw[LV_DL_NSLOT][16], bool have[LV_DL_NSLOT]) {
  auto &m = card ? sd : nvs; for (int i=0;i<LV_DL_NSLOT;++i) if (m.count(i)) { memcpy(raw[i], m[i].data(), 16); have[i]=true; } return card ? 1 : 0; }
static int sv(int i, const uint8_t raw[16]) { if (card && sdwrite_ok) { sd[i].assign(raw, raw+16); return 1; } nvs[i].assign(raw, raw+16); return 0; }
int main() {
  int fails = 0; auto CHECK=[&](bool c, const char *m){ printf("%s %s\n", c?"PASS":"FAIL", m); if(!c) ++fails; };
  lv_dl_io_load_hook = ld; lv_dl_io_save_hook = sv;
  lv_dl_boot();
  CHECK(lv_dl_phase == LV_DL_SLOTS, "entry lands on SAVE SLOTS");
  CHECK(!lv_dl_slot[0].used && !lv_dl_slot[1].used && !lv_dl_slot[2].used, "empty card -> 3x NEW GAME");
  CHECK(!lv_dl_exdata && lv_dl_sd_ok, "firmware path: no EX.DATA stamp, SD source");
  lv_dl_slot_i = 1; lv_dl_apply_slot(1);
  CHECK(lv_dl_phase == LV_DL_INTRO && lv_dl_round == 1, "NEW GAME -> level-1 typist");
  lv_dl_round = 4; lv_dl_score = 1234; lv_dl_play(); lv_dl_seen[0]=lv_dl_seen[2]=true; lv_dl_got = 1;
  lv_dl_open_pause(); CHECK(lv_dl_phase == LV_DL_PAUSE, "B -> PAUSE"); lv_dl_do_save();
  CHECK(sd.count(1) && sd[1][0]=='H' && sd[1][1]=='B' && sd[1][2]=='1' && sd[1][3]==0 && sd[1].size()==16, "slot2 -> SD 16-byte HB1 record");
  CHECK(sd[1][6]==4 && (sd[1][7]|(sd[1][8]<<8))==1234 && sd[1][9]==2 && sd[1][10]==1, "record = level 4, score 1234, towers 2, files 1");
  lv_dl_boot(); CHECK(lv_dl_slot[1].used && lv_dl_slot[1].level==4 && lv_dl_slot[1].score==1234, "reload from SD continues slot 2");
  lv_dl_apply_slot(1); CHECK(lv_dl_phase == LV_DL_PLAY && lv_dl_round==4 && lv_dl_got==1, "filled slot continues into PLAY with progress");
  card = false; lv_dl_open_pause(); lv_dl_do_save(); CHECK(nvs.count(1) && !lv_dl_sd_ok, "no SD -> NVS hbsl2 fallback, confirm shows NVS");
  lv_dl_boot(); CHECK(lv_dl_slot[1].used && lv_dl_slot[1].level==4, "no SD -> load from NVS");
  printf("%d fail\n", fails); return fails != 0;
}
