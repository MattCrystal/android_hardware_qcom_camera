/*
** Copyright (c) 2011-2012 Code Aurora Forum. All rights reserved.
**
** Not a Contribution, Apache license notifications and license are retained
** for attribution purposes only.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/*#error uncomment this for compiler test!*/

//#define ALOG_NDEBUG 0
#define ALOG_NIDEBUG 0
#define LOG_TAG "QCameraHWI_Record"
#include <utils/Log.h>
#include <utils/threads.h>
#include <cutils/properties.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "QCameraStream.h"


#define LIKELY(exp)   __builtin_expect(!!(exp), 1)
#define UNLIKELY(exp) __builtin_expect(!!(exp), 0)

/* QCameraStream_record class implementation goes here*/
/* following code implement the video streaming capture & encoding logic of this class*/
// ---------------------------------------------------------------------------
// QCameraStream_record createInstance()
// ---------------------------------------------------------------------------
namespace android {


QCameraStream* QCameraStream_record::createInstance(uint32_t CameraHandle,
                        uint32_t ChannelId,
                        uint32_t Width,
                        uint32_t Height,
                        uint32_t Format,
                        uint8_t NumBuffers,
                        mm_camera_vtbl_t *mm_ops,
                        mm_camera_img_mode imgmode,
                        camera_mode_t mode)
{
  ALOGV("%s: BEGIN", __func__);
  QCameraStream* pme = new QCameraStream_record(CameraHandle,
                        ChannelId,
                        Width,
                        Height,
                        Format,
                        NumBuffers,
                        mm_ops,
                        imgmode,
                        mode);
  ALOGV("%s: END", __func__);
  return pme;
}

// ---------------------------------------------------------------------------
// QCameraStream_record deleteInstance()
// ---------------------------------------------------------------------------
void QCameraStream_record::deleteInstance(QCameraStream *ptr)
{
  ALOGV("%s: BEGIN", __func__);
  if (ptr){
    ptr->release();
    delete ptr;
    ptr = NULL;
  }
  ALOGV("%s: END", __func__);
}

// ---------------------------------------------------------------------------
// QCameraStream_record Constructor
// ---------------------------------------------------------------------------
QCameraStream_record::QCameraStream_record(uint32_t CameraHandle,
                        uint32_t ChannelId,
                        uint32_t Width,
                        uint32_t Height,
                        uint32_t Format,
                        uint8_t NumBuffers,
                        mm_camera_vtbl_t *mm_ops,
                        mm_camera_img_mode imgmode,
                        camera_mode_t mode)
//  : QCameraStream(cameraId,mode),
  :QCameraStream(CameraHandle,
                 ChannelId,
                 Width,
                 Height,
                 Format,
                 NumBuffers,
                 mm_ops,
                 imgmode,
                 mode),
                 mDebugFps(false)
{
  mHalCamCtrl = NULL;
  char value[PROPERTY_VALUE_MAX];
  ALOGV("%s: BEGIN", __func__);

  property_get("persist.debug.sf.showfps", value, "0");
  mDebugFps = atoi(value);

  ALOGV("%s: END", __func__);
}

// ---------------------------------------------------------------------------
// QCameraStream_record Destructor
// ---------------------------------------------------------------------------
QCameraStream_record::~QCameraStream_record() {
  ALOGV("%s: BEGIN", __func__);
  if(mActive) {
    streamOff(0);
    deinitStream();
    //stop();
  }
  if(mInit) {
    release();
  }
  mInit = false;
  mActive = false;
  ALOGV("%s: END", __func__);

}

#if 0
// ---------------------------------------------------------------------------
// QCameraStream_record Callback from mm_camera
// ---------------------------------------------------------------------------
static void record_notify_cb(mm_camera_ch_data_buf_t *bufs_new,
                              void *user_data)
{
  QCameraStream_record *pme = (QCameraStream_record *)user_data;
  mm_camera_ch_data_buf_t *bufs_used = 0;
  ALOGV("%s: BEGIN", __func__);

  /*
  * Call Function Process Video Data
  */
  pme->processRecordFrame(bufs_new);
  ALOGV("%s: END", __func__);
}
#endif

#if 0
// ---------------------------------------------------------------------------
// QCameraStream_record
// ---------------------------------------------------------------------------
status_t QCameraStream_record::init()
{
  status_t ret = NO_ERROR;
  ALOGV("%s: BEGIN", __func__);

  /*
  *  Acquiring Video Channel
  */
  setFormat();
  ret = QCameraStream::initStream (0);
  if (NO_ERROR!=ret) {
    ALOGE("%s ERROR: Can't init native cammera preview ch\n",__func__);
    return ret;
  }

  mInit = true;
  ALOGV("%s: END", __func__);
  return ret;
}
#endif
// ---------------------------------------------------------------------------
// QCameraStream_record
// ---------------------------------------------------------------------------
#if 0
status_t QCameraStream_record::start()
{
  status_t ret = NO_ERROR;
  ALOGV("%s: BEGIN", __func__);

  Mutex::Autolock lock(mStopCallbackLock);
  if(!mInit) {
    ALOGE("%s ERROR: Record buffer not registered",__func__);
    return BAD_VALUE;
  }

  //mRecordFreeQueueLock.lock();
  //mRecordFreeQueue.clear();
  //mRecordFreeQueueLock.unlock();
  /*
  *  Allocating Encoder Frame Buffers
  */
#if 0
  ret = cam_config_prepare_buf(mCameraId, &mRecordBuf);
  if(ret != MM_CAMERA_OK) {
    ALOGV("%s ERROR: Reg Record buf err=%d\n", __func__, ret);
    ret = BAD_VALUE;
    goto error;
  }else{
    ret = NO_ERROR;
  }
#endif

  /*
  * Start Video Streaming
  */
  ret = streamOn();
  if (MM_CAMERA_OK != ret) {
    ALOGE ("%s ERROR: Video streaming start err=%d\n", __func__, ret);
    ret = BAD_VALUE;
    goto error;
  }else{
    ALOGE("%s : Video streaming Started",__func__);
    ret = NO_ERROR;
  }
  mActive = true;
  ALOGV("%s: END", __func__);
  return ret;

error:
//  releaseEncodeBuffer();
  ALOGV("%s: END", __func__);
  return ret;
}
#endif
void QCameraStream_record::releaseEncodeBuffer() {
  for(int cnt = 0; cnt < mHalCamCtrl->mRecordingMemory.buffer_count; cnt++) {
#if 0
    if (NO_ERROR !=
      mHalCamCtrl->sendUnMappingBuf(MSM_V4L2_EXT_CAPTURE_MODE_VIDEO, cnt,
      mCameraId, CAM_SOCK_MSG_TYPE_FD_UNMAPPING))
      ALOGE("%s: Unmapping Video Data Failed", __func__);
#endif
    if (mHalCamCtrl->mStoreMetaDataInFrame) {
      struct encoder_media_buffer_type * packet =
          (struct encoder_media_buffer_type  *)
          mHalCamCtrl->mRecordingMemory.metadata_memory[cnt]->data;
      native_handle_delete(const_cast<native_handle_t *>(packet->meta_handle));
      mHalCamCtrl->mRecordingMemory.metadata_memory[cnt]->release(
        mHalCamCtrl->mRecordingMemory.metadata_memory[cnt]);

    }
    mHalCamCtrl->mRecordingMemory.camera_memory[cnt]->release(
      mHalCamCtrl->mRecordingMemory.camera_memory[cnt]);
    close(mHalCamCtrl->mRecordingMemory.fd[cnt]);
    mHalCamCtrl->mRecordingMemory.fd[cnt] = -1;

#ifdef USE_ION
    mHalCamCtrl->deallocate_ion_memory(&mHalCamCtrl->mRecordingMemory, cnt);
#endif
  }
  memset(&mHalCamCtrl->mRecordingMemory, 0, sizeof(mHalCamCtrl->mRecordingMemory));
  //mNumRecordFrames = 0;
//  delete[] recordframes;
}

// ---------------------------------------------------------------------------
// QCameraStream_record
// ---------------------------------------------------------------------------
#if 0
void QCameraStream_record::stop()
{
  status_t ret = NO_ERROR;
  ALOGV("%s: BEGIN", __func__);

  if(!mActive) {
    ALOGE("%s : Record stream not started",__func__);
    return;
  }
  mActive =  false;
  Mutex::Autolock lock(mStopCallbackLock);
#if 0 //mzhu, when stop recording, all frame will be dirty. no need to queue frame back to kernel any more
  mRecordFreeQueueLock.lock();
  while(!mRecordFreeQueue.isEmpty()) {
    ALOGV("%s : Pre-releasing of Encoder buffers!\n", __FUNCTION__);
    mm_camera_ch_data_buf_t releasedBuf = mRecordFreeQueue.itemAt(0);
    mRecordFreeQueue.removeAt(0);
    mRecordFreeQueueLock.unlock();
    ALOGV("%s (%d): releasedBuf.idx = %d\n", __FUNCTION__, __LINE__,
                                              releasedBuf.video.video.idx);
    if(MM_CAMERA_OK != cam_evt_buf_done(mCameraId,&releasedBuf))
        ALOGE("%s : Buf Done Failed",__func__);
  }
  mRecordFreeQueueLock.unlock();
#if 0
  while (!mRecordFreeQueue.isEmpty()) {
        ALOGE("%s : Waiting for Encoder to release all buffer!\n", __FUNCTION__);
  }
#endif
#endif // mzhu
  /* unregister the notify fn from the mmmm_camera_t object
   *  call stop() in parent class to stop the monitor thread */

  ret = streamOff(0);
  if (MM_CAMERA_OK != ret) {
    ALOGE ("%s ERROR: Video streaming Stop err=%d\n", __func__, ret);
  }
#if 0
  ret = cam_config_unprepare_buf(mCameraId, MM_CAMERA_CH_VIDEO);
  if(ret != MM_CAMERA_OK){
    ALOGE("%s ERROR: Ureg video buf \n", __func__);
  }
#endif
//  releaseEncodeBuffer();

  mActive = false;
  ALOGV("%s: END", __func__);

}
#endif
// ---------------------------------------------------------------------------
// QCameraStream_record
// ---------------------------------------------------------------------------
void QCameraStream_record::release()
{
  status_t ret = NO_ERROR;
  ALOGV("%s: BEGIN", __func__);

  if(mActive) {
    streamOff(0);
  }
  if(mInit) {
    deinitStream();
  }

  mInit = false;
  ALOGV("%s: END", __func__);
}

status_t QCameraStream_record::processRecordFrame(mm_camera_super_buf_t *frame)
{
    ALOGV("%s : BEGIN",__func__);

    Mutex::Autolock lock(mStopCallbackLock);
    if(!mActive) {
      ALOGE("Recording Stopped. Returning callback");
      return NO_ERROR;
    }

    if (UNLIKELY(mDebugFps)) {
        debugShowVideoFPS();
    }
    ALOGE("DEBUG4:%d",frame->bufs[0]->stream_id);
    ALOGE("<DEBUG4>: Timestamp: %ld %ld",frame->bufs[0]->ts.tv_sec,frame->bufs[0]->ts.tv_nsec);
    mHalCamCtrl->dumpFrameToFile(frame->bufs[0], HAL_DUMP_FRM_VIDEO);
    mHalCamCtrl->mCallbackLock.lock();
    camera_data_timestamp_callback rcb = mHalCamCtrl->mDataCbTimestamp;
    void *rdata = mHalCamCtrl->mCallbackCookie;
    mHalCamCtrl->mCallbackLock.unlock();
	nsecs_t timeStamp = nsecs_t(frame->bufs[0]->ts.tv_sec)*1000000000LL + \
                      frame->bufs[0]->ts.tv_nsec;

  ALOGE("Send Video frame to services/encoder TimeStamp : %lld",timeStamp);
  mRecordedFrames[frame->bufs[0]->buf_idx] = *frame;

#if 0
#ifdef USE_ION
  struct ion_flush_data cache_inv_data;
  int ion_fd;
  ion_fd = frame->bufs[0]->frame->ion_dev_fd;
  cache_inv_data.vaddr = (void *)frame->bufs[0]->frame->buffer;
  cache_inv_data.fd = frame->bufs[0]->frame->fd;
  cache_inv_data.handle = frame->bufs[0]->frame->fd_data.handle;
  cache_inv_data.length = frame->bufs[0]->frame->ion_alloc.len;

  if (mHalCamCtrl->cache_ops(ion_fd, &cache_inv_data, ION_IOC_CLEAN_CACHES) < 0)
    ALOGE("%s: Cache clean for Video buffer %p fd = %d failed", __func__,
      cache_inv_data.vaddr, cache_inv_data.fd);
#endif
#endif

  if (mHalCamCtrl->mStoreMetaDataInFrame) {
    mStopCallbackLock.unlock();
    if(mActive && (rcb != NULL) && (mHalCamCtrl->mMsgEnabled & CAMERA_MSG_VIDEO_FRAME)) {
      ALOGE("Calling video callback:%d",frame->bufs[0]->buf_idx);
      rcb(timeStamp, CAMERA_MSG_VIDEO_FRAME,
              mHalCamCtrl->mRecordingMemory.metadata_memory[frame->bufs[0]->buf_idx],
              0, mHalCamCtrl->mCallbackCookie);
    }
  } else {
    mStopCallbackLock.unlock();
    if(mActive && (rcb != NULL) && (mHalCamCtrl->mMsgEnabled & CAMERA_MSG_VIDEO_FRAME)) {
      ALOGE("Calling video callback2");
      rcb(timeStamp, CAMERA_MSG_VIDEO_FRAME,
              mHalCamCtrl->mRecordingMemory.camera_memory[frame->bufs[0]->buf_idx],
              0, mHalCamCtrl->mCallbackCookie);
    }
  }

  ALOGV("%s : END",__func__);
  return NO_ERROR;
}

//Record Related Functions
status_t QCameraStream_record::initEncodeBuffers()
{
  ALOGE("%s : BEGIN",__func__);
  status_t ret = NO_ERROR;
  const char *pmem_region;
  uint32_t frame_len=mFrameOffsetInfo.frame_len;
  uint8_t num_planes=mFrameOffsetInfo.num_planes;
  uint32_t planes[VIDEO_MAX_PLANES];
  //cam_ctrl_dimension_t dim;
  int width = mWidth;  /* width of channel  */
  int height = mHeight; /* height of channel */
  int buf_cnt;
  pmem_region = "/dev/pmem_adsp";


  memset(&mHalCamCtrl->mRecordingMemory, 0, sizeof(mHalCamCtrl->mRecordingMemory));

#if 0
  memset(&dim, 0, sizeof(cam_ctrl_dimension_t));
  ret = cam_config_get_parm(mCameraId, MM_CAMERA_PARM_DIMENSION, &dim);
  if (MM_CAMERA_OK != ret) {
    ALOGE("%s: ERROR - can't get camera dimension!", __func__);
    return BAD_VALUE;
  }
  else {
    width =  dim.video_width;
    height = dim.video_height;
  }
  num_planes = 2;
#endif
  for(int i=0;i<num_planes;i++)
    planes[i] = mFrameOffsetInfo.mp[i].len;


  buf_cnt = VIDEO_BUFFER_COUNT;
  if(mHalCamCtrl->isLowPowerCamcorder()) {
    ALOGE("%s: lower power camcorder selected", __func__);
    buf_cnt = VIDEO_BUFFER_COUNT_LOW_POWER_CAMCORDER;
  }
//    recordframes = new msm_frame[buf_cnt];
//    memset(recordframes,0,sizeof(struct msm_frame) * buf_cnt);
    memset(mRecordBuf, 0, 2 * VIDEO_BUFFER_COUNT * sizeof(mm_camera_buf_def_t));

    memset(&mHalCamCtrl->mRecordingMemory, 0, sizeof(mHalCamCtrl->mRecordingMemory));
    for (int i=0; i<MM_CAMERA_MAX_NUM_FRAMES;i++) {
        mHalCamCtrl->mRecordingMemory.main_ion_fd[i] = -1;
        mHalCamCtrl->mRecordingMemory.fd[i] = -1;
    }

    mHalCamCtrl->mRecordingMemory.buffer_count = buf_cnt;

		mHalCamCtrl->mRecordingMemory.size = frame_len;
		mHalCamCtrl->mRecordingMemory.cbcr_offset = planes[0];

    for (int cnt = 0; cnt < mHalCamCtrl->mRecordingMemory.buffer_count; cnt++) {
#ifdef USE_ION
      if(mHalCamCtrl->allocate_ion_memory(&mHalCamCtrl->mRecordingMemory, cnt,
        ((0x1 << CAMERA_ION_HEAP_ID) | (0x1 << CAMERA_ION_FALLBACK_HEAP_ID))) < 0) {
        ALOGE("%s ION alloc failed\n", __func__);
        return UNKNOWN_ERROR;
      }
#else
		  mHalCamCtrl->mRecordingMemory.fd[cnt] = open("/dev/pmem_adsp", O_RDWR|O_SYNC);
		  if(mHalCamCtrl->mRecordingMemory.fd[cnt] <= 0) {
			  ALOGE("%s: no pmem for frame %d", __func__, cnt);
			  return UNKNOWN_ERROR;
		  }
#endif
		  mHalCamCtrl->mRecordingMemory.camera_memory[cnt] =
		    mHalCamCtrl->mGetMemory(mHalCamCtrl->mRecordingMemory.fd[cnt],
		    mHalCamCtrl->mRecordingMemory.size, 1, (void *)this);

      if (mHalCamCtrl->mStoreMetaDataInFrame) {
        mHalCamCtrl->mRecordingMemory.metadata_memory[cnt] =
          mHalCamCtrl->mGetMemory(-1,
          sizeof(struct encoder_media_buffer_type), 1, (void *)this);
        struct encoder_media_buffer_type * packet =
          (struct encoder_media_buffer_type  *)
          mHalCamCtrl->mRecordingMemory.metadata_memory[cnt]->data;
        packet->meta_handle = native_handle_create(1, 2); //1 fd, 1 offset and 1 size
        packet->buffer_type = kMetadataBufferTypeCameraSource;
        native_handle_t * nh = const_cast<native_handle_t *>(packet->meta_handle);
        nh->data[0] = mHalCamCtrl->mRecordingMemory.fd[cnt];
        nh->data[1] = 0;
        nh->data[2] = mHalCamCtrl->mRecordingMemory.size;
      }
//    	recordframes[cnt].fd = mHalCamCtrl->mRecordingMemory.fd[cnt];
//    	recordframes[cnt].buffer = (uint32_t)mHalCamCtrl->mRecordingMemory.camera_memory[cnt]->data;
//	    recordframes[cnt].y_off = 0;
//	    recordframes[cnt].cbcr_off = mHalCamCtrl->mRecordingMemory.cbcr_offset;
//	    recordframes[cnt].path = OUTPUT_TYPE_V;
//      recordframes[cnt].fd_data = mHalCamCtrl->mRecordingMemory.ion_info_fd[cnt];
//      recordframes[cnt].ion_alloc = mHalCamCtrl->mRecordingMemory.alloc[cnt];
//      recordframes[cnt].ion_dev_fd = mHalCamCtrl->mRecordingMemory.main_ion_fd[cnt];

#if 0
      if (NO_ERROR !=
        mHalCamCtrl->sendMappingBuf(MSM_V4L2_EXT_CAPTURE_MODE_VIDEO, cnt,
        recordframes[cnt].fd, mHalCamCtrl->mRecordingMemory.size, mCameraId,
        CAM_SOCK_MSG_TYPE_FD_MAPPING))
        ALOGE("%s: sending mapping data Msg Failed", __func__);
#endif
      ALOGE ("initRecord :  record heap , video buffers  buffer=%lu fd=%d y_off=%d cbcr_off=%d\n",
		    (unsigned long)(uint32_t)mHalCamCtrl->mRecordingMemory.camera_memory[cnt]->data,mHalCamCtrl->mRecordingMemory.fd[cnt],0 ,
		mHalCamCtrl->mRecordingMemory.cbcr_offset);
	    //mNumRecordFrames++;

//      mRecordBuf[cnt].frame = &recordframes[cnt];
//      mRecordBuf[cnt].frame_offset = 0;
      mRecordBuf[cnt].num_planes = num_planes;
      /* Plane 0 needs to be set seperately. Set other planes
       * in a loop. */
      mRecordBuf[cnt].planes[0].reserved[0] = 0;
      mRecordBuf[cnt].planes[0].length = planes[0];
      mRecordBuf[cnt].planes[0].m.userptr =
       mHalCamCtrl->mRecordingMemory.fd[cnt];
      for (int j = 1; j < num_planes; j++) {
        mRecordBuf[cnt].planes[j].length = planes[j];
        mRecordBuf[cnt].planes[j].m.userptr =mHalCamCtrl->mRecordingMemory.fd[cnt];
        mRecordBuf[cnt].planes[j].reserved[0] =
          mRecordBuf[cnt].planes[j-1].reserved[0] +
          mRecordBuf[cnt].planes[j-1].length;
      }
      mRecordBuf[cnt].stream_id = mStreamId;
      mRecordBuf[cnt].fd = mHalCamCtrl->mRecordingMemory.fd[cnt];
      mRecordBuf[cnt].frame_len = mFrameOffsetInfo.frame_len;
    }

    //memset(&mRecordBuf, 0, sizeof(mRecordBuf));
    //mRecordBuf.video.video.frame_offset = &record_offset[0];
    //mRecordBuf.video.video.frame = &recordframes[0];
    ALOGE("%s : END",__func__);
    return NO_ERROR;
}

void QCameraStream_record::releaseRecordingFrame(const void *opaque)
{
    ALOGV("%s : BEGIN, opaque = 0x%p",__func__, opaque);
    if(!mActive)
    {
        ALOGE("%s : Recording already stopped!!! Leak???",__func__);
        return;
    }
    for(int cnt = 0; cnt < mHalCamCtrl->mRecordingMemory.buffer_count; cnt++) {
      if (mHalCamCtrl->mStoreMetaDataInFrame) {
        if(mHalCamCtrl->mRecordingMemory.metadata_memory[cnt] &&
                mHalCamCtrl->mRecordingMemory.metadata_memory[cnt]->data == opaque) {
            /* found the match */
            ALOGE("Releasing video frames:%d",cnt);
            if(MM_CAMERA_OK != p_mm_ops->ops->qbuf(mCameraHandle,mChannelId, mRecordedFrames[cnt].bufs[0]))
                ALOGE("%s : Buf Done Failed",__func__);
            ALOGV("%s : END",__func__);
            return;
        }
      } else {
        if(mHalCamCtrl->mRecordingMemory.camera_memory[cnt] &&
                mHalCamCtrl->mRecordingMemory.camera_memory[cnt]->data == opaque) {
            /* found the match */
            ALOGE("Releasing video frames2");
            if(MM_CAMERA_OK != p_mm_ops->ops->qbuf(mCameraHandle,mChannelId, mRecordedFrames[cnt].bufs[0]))
                ALOGE("%s : Buf Done Failed",__func__);
            ALOGV("%s : END",__func__);
            return;
        }
      }
    }
	ALOGE("%s: cannot find the matched frame with opaue = 0x%p", __func__, opaque);
}

void QCameraStream_record::debugShowVideoFPS() const
{
  static int mFrameCount;
  static int mLastFrameCount = 0;
  static nsecs_t mLastFpsTime = 0;
  static float mFps = 0;
  mFrameCount++;
  nsecs_t now = systemTime();
  nsecs_t diff = now - mLastFpsTime;
  if (diff > ms2ns(250)) {
    mFps =  ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
    ALOGI("Video Frames Per Second: %.4f", mFps);
    mLastFpsTime = now;
    mLastFrameCount = mFrameCount;
  }
}

#if 0
sp<IMemoryHeap> QCameraStream_record::getHeap() const
{
  return mRecordHeap != NULL ? mRecordHeap->mHeap : NULL;
}

#endif
status_t  QCameraStream_record::takeLiveSnapshot(){
	return true;
}

}//namespace android

