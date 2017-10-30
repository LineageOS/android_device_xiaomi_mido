/* Copyright (c) 2011-2017, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LOC_API_V_0_2_H
#define LOC_API_V_0_2_H

#include <stdint.h>
#include <stdbool.h>
#include <ds_client.h>
#include <LocApiBase.h>
#include <loc_api_v02_client.h>

using namespace loc_core;

/* This class derives from the LocApiBase class.
   The members of this class are responsible for converting
   the Loc API V02 data structures into Loc Adapter data structures.
   This class also implements some of the virtual functions that
   handle the requests from loc engine. */
class LocApiV02 : public LocApiBase {
  enum supported_status {
      sup_unknown,
      sup_yes,
      sup_no
  };
protected:
  /* loc api v02 handle*/
  locClientHandleType clientHandle;

private:
  /* ds client library handle */
  void *dsLibraryHandle;
  /* ds client interface */
  const ds_client_iface_type *dsClientIface;
  /* ds client handle */
  dsClientHandleType dsClientHandle;
  enum supported_status mGnssMeasurementSupported;
  locClientEventMaskType mQmiMask;
  bool mInSession;
  bool mEngineOn;
  bool mMeasurementsStarted;

  /* Convert event mask from loc eng to loc_api_v02 format */
  static locClientEventMaskType convertMask(LOC_API_ADAPTER_EVENT_MASK_T mask);

  /* Convert GPS LOCK mask */
  static qmiLocLockEnumT_v02 convertGpsLockMask(GnssConfigGpsLock lock);

  /* Convert error from loc_api_v02 to loc eng format*/
  static enum loc_api_adapter_err convertErr(locClientStatusEnumType status);

  /* convert Ni Encoding type from QMI_LOC to loc eng format */
  static GnssNiEncodingType convertNiEncoding(
    qmiLocNiDataCodingSchemeEnumT_v02 loc_encoding);

  /*convert NI notify verify type from QMI LOC to loc eng format*/
  static bool convertNiNotifyVerifyType (GnssNiNotification *notif,
      qmiLocNiNotifyVerifyEnumT_v02 notif_priv);

  /*convert GnssMeasurement type from QMI LOC to loc eng format*/
  static void convertGnssMeasurements (GnssMeasurementsData& measurementData,
      const qmiLocEventGnssSvMeasInfoIndMsgT_v02& gnss_measurement_report_ptr,
      int index);

  /*convert LocGnssClock type from QMI LOC to loc eng format*/
  int convertGnssClock (GnssMeasurementsClock& clock,
      const qmiLocEventGnssSvMeasInfoIndMsgT_v02& gnss_measurement_info);

  /* If Confidence value is less than 68%, then scale the accuracy value to 68%
     confidence.*/
  void scaleAccuracyTo68PercentConfidence(const uint8_t confidenceValue,
                                          LocGpsLocation &gpsLocation,
                                          const bool isCircularUnc);

  /* convert position report to loc eng format and send the converted
     position to loc eng */
  void reportPosition
    (const qmiLocEventPositionReportIndMsgT_v02 *location_report_ptr);

  /* convert satellite report to loc eng format and  send the converted
     report to loc eng */
  void reportSv (const qmiLocEventGnssSvInfoIndMsgT_v02 *gnss_report_ptr);

  void reportSvMeasurement (
  const qmiLocEventGnssSvMeasInfoIndMsgT_v02 *gnss_raw_measurement_ptr);

  void  reportSvPolynomial (
  const qmiLocEventGnssSvPolyIndMsgT_v02 *gnss_sv_poly_ptr);

  /* convert engine state report to loc eng format and send the converted
     report to loc eng */
  void reportEngineState (
    const qmiLocEventEngineStateIndMsgT_v02 *engine_state_ptr);

  /* convert fix session report to loc eng format and send the converted
     report to loc eng */
  void reportFixSessionState (
    const qmiLocEventFixSessionStateIndMsgT_v02 *fix_session_state_ptr);

  /* convert NMEA report to loc eng format and send the converted
     report to loc eng */
  void reportNmea (const qmiLocEventNmeaIndMsgT_v02 *nmea_report_ptr);

  /* convert and report an ATL request to loc engine */
  void reportAtlRequest(
    const qmiLocEventLocationServerConnectionReqIndMsgT_v02
    *server_request_ptr);

  /* convert and report NI request to loc eng */
  void reportNiRequest(
    const qmiLocEventNiNotifyVerifyReqIndMsgT_v02 *ni_req_ptr);

  /* report the xtra server info */
  void reportXtraServerUrl(
    const qmiLocEventInjectPredictedOrbitsReqIndMsgT_v02* server_request_ptr);

  /* convert and report GNSS measurement data to loc eng */
  void reportGnssMeasurementData(
    const qmiLocEventGnssSvMeasInfoIndMsgT_v02& gnss_measurement_report_ptr);

  bool registerEventMask(locClientEventMaskType qmiMask);
  locClientEventMaskType adjustMaskForNoSession(locClientEventMaskType qmiMask);
  void cacheGnssMeasurementSupport();

protected:
  virtual enum loc_api_adapter_err
    open(LOC_API_ADAPTER_EVENT_MASK_T mask);
  virtual enum loc_api_adapter_err
    close();

  LocApiV02(const MsgTask* msgTask,
            LOC_API_ADAPTER_EVENT_MASK_T exMask,
            ContextBase *context = NULL);
public:
  ~LocApiV02();

  static LocApiBase* createLocApiV02(const MsgTask *msgTask,
                                  LOC_API_ADAPTER_EVENT_MASK_T exMask,
                                  ContextBase* context);
  /* event callback registered with the loc_api v02 interface */
  virtual void eventCb(locClientHandleType client_handle,
               uint32_t loc_event_id,
               locClientEventIndUnionType loc_event_payload);

  /* error callback, this function handles the  service unavailable
     error */
  void errorCb(locClientHandleType handle,
               locClientErrorEnumType errorId);

  void ds_client_event_cb(ds_client_status_enum_type result);

  virtual enum loc_api_adapter_err startFix(const LocPosMode& posMode);

  virtual enum loc_api_adapter_err stopFix();

  virtual enum loc_api_adapter_err
    setPositionMode(const LocPosMode& mode);

  virtual enum loc_api_adapter_err
    setTime(LocGpsUtcTime time, int64_t timeReference, int uncertainty);

  virtual enum loc_api_adapter_err
    injectPosition(double latitude, double longitude, float accuracy);

  virtual LocationError
    deleteAidingData(const GnssAidingData& data);

  virtual LocationError
    informNiResponse(GnssNiResponse userResponse, const void* passThroughData);

  virtual LocationError
    setServer(const char* url, int len);
  virtual LocationError
    setServer(unsigned int ip, int port, LocServerType type);
  virtual enum loc_api_adapter_err
    setXtraData(char* data, int length);
  virtual enum loc_api_adapter_err
    requestXtraServer();
  virtual enum loc_api_adapter_err
    atlOpenStatus(int handle, int is_succ, char* apn, AGpsBearerType bear,
                   LocAGpsType agpsType);
  virtual enum loc_api_adapter_err atlCloseStatus(int handle, int is_succ);
  virtual LocationError setSUPLVersion(GnssConfigSuplVersion version);

  virtual enum loc_api_adapter_err setNMEATypes (uint32_t typesMask);

  virtual LocationError setLPPConfig(GnssConfigLppProfile profile);

  virtual enum loc_api_adapter_err
    setSensorControlConfig(int sensorUsage, int sensorProvider);

  virtual enum loc_api_adapter_err
    setSensorProperties(bool gyroBiasVarianceRandomWalk_valid, float gyroBiasVarianceRandomWalk,
                            bool accelBiasVarianceRandomWalk_valid, float accelBiasVarianceRandomWalk,
                            bool angleBiasVarianceRandomWalk_valid, float angleBiasVarianceRandomWalk,
                            bool rateBiasVarianceRandomWalk_valid, float rateBiasVarianceRandomWalk,
                            bool velocityBiasVarianceRandomWalk_valid, float velocityBiasVarianceRandomWalk);

  virtual enum loc_api_adapter_err
    setSensorPerfControlConfig(int controlMode, int accelSamplesPerBatch, int accelBatchesPerSec,
                               int gyroSamplesPerBatch, int gyroBatchesPerSec,
                               int accelSamplesPerBatchHigh, int accelBatchesPerSecHigh,
                               int gyroSamplesPerBatchHigh, int gyroBatchesPerSecHigh, int algorithmConfig);
  virtual LocationError
      setAGLONASSProtocol(GnssConfigAGlonassPositionProtocolMask aGlonassProtocol);
  virtual LocationError setLPPeProtocolCp(GnssConfigLppeControlPlaneMask lppeCP);
  virtual LocationError setLPPeProtocolUp(GnssConfigLppeUserPlaneMask lppeUP);
  virtual enum loc_api_adapter_err
      getWwanZppFix();
  virtual void
      handleWwanZppFixIndication(const qmiLocGetAvailWwanPositionIndMsgT_v02 &zpp_ind);
  virtual enum loc_api_adapter_err
      getBestAvailableZppFix(LocGpsLocation & zppLoc);
  virtual enum loc_api_adapter_err
      getBestAvailableZppFix(LocGpsLocation & zppLoc, GpsLocationExtended & location_extended,
              LocPosTechMask & tech_mask);
  virtual int initDataServiceClient(bool isDueToSsr);
  virtual int openAndStartDataCall();
  virtual void stopDataCall();
  virtual void closeDataCall();
  virtual void releaseDataServiceClient();
  virtual LocationError setGpsLock(GnssConfigGpsLock lock);

  /*
    Returns
    Current value of GPS Lock on success
    -1 on failure
  */
  virtual int getGpsLock(void);
  virtual int setSvMeasurementConstellation(const qmiLocGNSSConstellEnumT_v02 svConstellation);
  virtual LocationError setXtraVersionCheck(uint32_t check);
  virtual void installAGpsCert(const LocDerEncodedCertificate* pData,
                               size_t length,
                               uint32_t slotBitMask);
  /*
    Set Gnss Constellation Config
  */
  virtual bool gnssConstellationConfig();
  virtual LocPosTechMask convertPosTechMask(qmiLocPosTechMaskT_v02 mask);
  virtual LocNavSolutionMask convertNavSolutionMask(qmiLocNavSolutionMaskT_v02 mask);

};

extern "C" LocApiBase* getLocApi(const MsgTask* msgTask,
                                 LOC_API_ADAPTER_EVENT_MASK_T exMask,
                                 ContextBase *context);
#endif //LOC_API_V_0_2_H
