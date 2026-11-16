/* THREAT BRAIN mock: scripted scan feed. EXAMPLE DATA ONLY - invented hit times, not radio output.
 * Times in seconds from the start of the 9 s clip; negative = hits just before the clip (so the brain starts warm).
 * Story: an AirTag is following the user (steady hits), a deauth flood bursts at ~2 s and ~7 s, Flock cameras
 * pass at an intersection, a few SmartTags and open Wi-Fi networks, one drone, one skimmer-style HC-05, no Flipper. */
#pragma once
static const float TBF_DEAUTH[]  = {2.0f, 2.15f, 2.3f, 2.45f, 2.6f, 2.8f, 3.05f, 3.3f, 7.1f, 7.3f, 7.5f};
static const float TBF_META[]    = {3.6f, 8.1f};
static const float TBF_HACK[]    = {-2.0f, 0.9f, 5.5f};
static const float TBF_SKIMMER[] = {6.5f};
static const float TBF_FLOCK[]   = {1.0f, 1.35f, 2.6f, 5.9f, 6.2f, 8.4f};
static const float TBF_FLIPPER[] = {99.f};   /* none in the clip */
static const float TBF_DRONE[]   = {7.0f};
static const float TBF_AIRTAG[]  = {-3.0f, -1.2f, 0.4f, 1.9f, 3.1f, 4.4f, 5.2f, 6.8f, 7.9f};
static const float TBF_KEYFOB[]  = {4.8f};
static const float TBF_SAMSUNG[] = {-1.0f, 1.6f, 4.1f, 6.3f};
#define TBF_N(a) (int)(sizeof a / sizeof a[0])
static const TbFeed TB_FEED_EX = {{TBF_DEAUTH, TBF_META, TBF_HACK, TBF_SKIMMER, TBF_FLOCK, TBF_FLIPPER, TBF_DRONE, TBF_AIRTAG, TBF_KEYFOB, TBF_SAMSUNG},
                                  {TBF_N(TBF_DEAUTH), TBF_N(TBF_META), TBF_N(TBF_HACK), TBF_N(TBF_SKIMMER), TBF_N(TBF_FLOCK), 0, TBF_N(TBF_DRONE),
                                   TBF_N(TBF_AIRTAG), TBF_N(TBF_KEYFOB), TBF_N(TBF_SAMSUNG)}};
