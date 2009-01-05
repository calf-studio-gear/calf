/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 *  This work is in public domain.
 *
 *  This file is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  Author of this extension is Nedko Arnaudov <nedko@arnaudov.name>
 *
 *  Several people helped improving the extension:
 *   * Krzysztof Foltman <wdev@foltman.com>
 *   * Dave Robillard <dave@drobilla.net>
 *
 *  If you have questions ask in the #lv2 or #lad channel,
 *  FreeNode IRC network or use the lv2 mailing list.
 *
 *****************************************************************************/

#ifndef LV2_PROGRESS_H__F576843C_CA13_49C3_9BF9_CFF3A15AD18C__INCLUDED
#define LV2_PROGRESS_H__F576843C_CA13_49C3_9BF9_CFF3A15AD18C__INCLUDED

/**
 * @file lv2_progress.h
 * @brief LV2 progress notification extension definition
 *
 * @par Purpose and scope
 * The purpose of this extension is to prevent thread (often the main one)
 * freeze for plugins doing intensive computations during instantiation.
 * Host may want to display progress bar of some sort and if it is using
 * GUI toolkit event loop, to dispatch queued events.
 *
 * @par
 * Using this extension for reporting progress of other lengthy operations
 * is possible and encouraged but not subject of this extension alone.
 * It is probably not good idea to call progress callback from plugin created
 * thread nor it is good idea to call it from a LV2 audio class function.
 * Any extension that wants to work in conjuction with progress extension
 * must define thread context semantics for calling the progress callback.
 *
 * @par Plugin instantiation progress usage
 * Plugin calls the host provided callback on regular basis during
 * instantiation. One second between calls is good target. Everything between
 * half second and two seconds should provide enough motion so user does
 * not get "the thing froze" impression.
 */

#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* Adjust editor indent */
#endif

/** URI for the plugin progress feature */
#define LV2_PROGRESS_URI "http://lv2plug.in/ns/dev/progress"

/** @brief host feature structure */
typedef struct _LV2_Progress
{
  /** to be supplied as first parameter of progress() callback  */
  void * context;

  /**
   * This function is called by plugin to notify host about progress of a
   * lengthy operation.
   *
   * @param context Host context
   * @param progress Progress, from 0.0 to 100.0
   * @param message Optional (may be NULL) string describing current operation.
   * If called once with non-NULL message, subsequent calls will NULL message
   * mean that host will reuse the previous message.
   */
  void (*progress)(void * context, float progress, const char * message);
} LV2_Progress;

#if 0
{ /* Adjust editor indent */
#endif
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* #ifndef LV2_PROGRESS_H__F576843C_CA13_49C3_9BF9_CFF3A15AD18C__INCLUDED */
