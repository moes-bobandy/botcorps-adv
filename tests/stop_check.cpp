#include <assert.h>
#include <stdio.h>
static int stops=0,prints=0;enum{LV_J_NONE};static int lv_job=0,lv_hunt=3;static bool lv_dirty_box=false;
struct Radio{void(*stop)();};static void stop(){++stops;}static Radio radio{stop};static Radio*lv_radio=&radio;
static const char*lv_job_name(int){return "WIFI";}template<class...T>void lv_print(const char*,T...){++prints;}
static void lv_job_stop(void)
{
    lv_radio->stop(); // STOP also releases a warm worker after the UI has consumed a one-shot scan.
    if (lv_job == LV_J_NONE) return;
    lv_print("stopped %s, radios off", lv_job_name(lv_job));
    lv_job = LV_J_NONE; lv_hunt = -1; lv_dirty_box = true;
}
int main(){lv_job_stop();assert(stops==1&&prints==0);lv_job=1;lv_job_stop();assert(stops==2&&prints==1&&lv_job==0&&lv_hunt==-1&&lv_dirty_box);puts("STOP lifecycle PASS: warm idle and active job both release radio; no extra idle UI log");}
