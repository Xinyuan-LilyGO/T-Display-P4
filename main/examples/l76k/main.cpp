/*
 * @Description: l76k
 * @Author: LILYGO_L
 * @Date: 2025-06-13 13:32:01
 * @LastEditTime: 2026-05-15 18:23:28
 * @License: GPL 3.0
 */
#include "lilygo_device_driver_library.h"

namespace {

bool IsUpdatedFloat(float value) { return value >= 0.0f; }

void PrintLocation(
    const char* title, const cpp_bus_driver::GnssParser::Location& location) {
  if (location.lat.update_flag && location.lat.direction_update_flag) {
    printf(
        "%s lat: %u deg %.10f min %.10lf deg %s\n", title,
        static_cast<unsigned int>(location.lat.degrees), location.lat.minutes,
        location.lat.degrees_minutes, location.lat.direction.c_str());
  }

  if (location.lon.update_flag && location.lon.direction_update_flag) {
    printf(
        "%s lon: %u deg %.10f min %.10lf deg %s\n", title,
        static_cast<unsigned int>(location.lon.degrees), location.lon.minutes,
        location.lon.degrees_minutes, location.lon.direction.c_str());
  }
}

void PrintUtc(const char* title, const cpp_bus_driver::GnssParser::Utc& utc) {
  if (!utc.update_flag) {
    return;
  }

  printf("%s UTC: %02u:%02u:%06.3f\n", title,
      static_cast<unsigned int>(utc.hour),
      static_cast<unsigned int>(utc.minute), utc.second);
  printf("%s China: %02u:%02u:%06.3f\n", title,
      static_cast<unsigned int>((utc.hour + 8 + 24) % 24),
      static_cast<unsigned int>(utc.minute), utc.second);
}

void PrintDate(const char* title, const cpp_bus_driver::GnssParser::Date& date) {
  if (!date.update_flag) {
    return;
  }

  printf("%s date: %u/%02u/%02u\n", title,
      static_cast<unsigned int>(date.year),
      static_cast<unsigned int>(date.month),
      static_cast<unsigned int>(date.day));
}

void PrintRmc(const cpp_bus_driver::L76k::Rmc& rmc) {
  printf("------------RMC------------\n");
  if (rmc.location_status_update_flag) {
    printf("Status: %s\n", rmc.location_status.c_str());
  }
  PrintUtc("RMC", rmc.utc);
  if (rmc.data.update_flag) {
    printf("RMC date: %u/%02u/%02u\n",
        static_cast<unsigned int>(rmc.data.year + 2000),
        static_cast<unsigned int>(rmc.data.month),
        static_cast<unsigned int>(rmc.data.day));
  }
  PrintLocation("RMC", rmc.location);
  if (IsUpdatedFloat(rmc.speed_over_ground_knots)) {
    printf("Speed: %.3f kn\n", rmc.speed_over_ground_knots);
  }
  if (IsUpdatedFloat(rmc.course_over_ground_degree)) {
    printf("Course: %.3f deg\n", rmc.course_over_ground_degree);
  }
  if (IsUpdatedFloat(rmc.magnetic_variation)) {
    printf("Magnetic variation: %.3f %s\n", rmc.magnetic_variation,
        rmc.magnetic_variation_direction.c_str());
  }
  if (!rmc.mode_indicator.empty()) {
    printf("Mode: %s\n", rmc.mode_indicator.c_str());
  }
  if (!rmc.navigational_status.empty()) {
    printf("Navigation status: %s\n", rmc.navigational_status.c_str());
  }
}

void PrintGga(const cpp_bus_driver::L76k::Gga& gga) {
  printf("------------GGA------------\n");
  PrintUtc("GGA", gga.utc);
  PrintLocation("GGA", gga.location);
  if (gga.gps_mode_status != 0xFF) {
    printf("Fix quality: %u\n", static_cast<unsigned int>(gga.gps_mode_status));
  }
  if (gga.online_satellite_count != 0xFF) {
    printf("Satellites used: %u\n",
        static_cast<unsigned int>(gga.online_satellite_count));
  }
  if (IsUpdatedFloat(gga.hdop)) {
    printf("HDOP: %.3f\n", gga.hdop);
  }
  if (!gga.altitude_unit.empty()) {
    printf("Altitude: %.3f %s\n", gga.altitude, gga.altitude_unit.c_str());
  }
  if (!gga.geoid_separation_unit.empty()) {
    printf("Geoid separation: %.3f %s\n", gga.geoid_separation,
        gga.geoid_separation_unit.c_str());
  }
  if (IsUpdatedFloat(gga.differential_age)) {
    printf("DGPS age: %.3f\n", gga.differential_age);
  }
  if (!gga.differential_station_id.empty()) {
    printf("DGPS station: %s\n", gga.differential_station_id.c_str());
  }
}

void PrintGsv(const cpp_bus_driver::L76k::Gsv& gsv) {
  printf("------------GSV------------\n");
  if (!gsv.update_flag) {
    return;
  }

  printf("Talker: %s, sentence %u/%u, satellites in view: %u\n",
      gsv.talker_id.c_str(), static_cast<unsigned int>(gsv.sentence_number),
      static_cast<unsigned int>(gsv.total_sentence_count),
      static_cast<unsigned int>(gsv.total_satellite_count));
  for (const auto& satellite : gsv.satellites) {
    printf("Satellite %u: elevation %d, azimuth %d, C/N0 %d, signal %u\n",
        static_cast<unsigned int>(satellite.id),
        static_cast<int>(satellite.elevation),
        static_cast<int>(satellite.azimuth), static_cast<int>(satellite.cn0),
        static_cast<unsigned int>(satellite.signal_id));
  }
}

void PrintGsa(const cpp_bus_driver::L76k::Gsa& gsa) {
  printf("------------GSA------------\n");
  if (!gsa.update_flag) {
    return;
  }

  for (const auto& sentence : gsa.sentences) {
    printf("Talker: %s, selection: %s, fix mode: %u\n",
        sentence.talker_id.c_str(), sentence.selection_mode.c_str(),
        static_cast<unsigned int>(sentence.fix_mode));
    printf("Satellites used:");
    for (uint16_t satellite_id : sentence.satellite_ids) {
      printf(" %u", static_cast<unsigned int>(satellite_id));
    }
    printf("\nPDOP %.3f, HDOP %.3f, VDOP %.3f, system %u\n", sentence.pdop,
        sentence.hdop, sentence.vdop,
        static_cast<unsigned int>(sentence.system_id));
  }
}

void PrintVtg(const cpp_bus_driver::L76k::Vtg& vtg) {
  printf("------------VTG------------\n");
  if (!vtg.update_flag) {
    return;
  }

  printf("Course true: %.3f, magnetic: %.3f\n", vtg.course_true_degree,
      vtg.course_magnetic_degree);
  printf("Speed: %.3f kn, %.3f km/h\n", vtg.speed_knots, vtg.speed_kmh);
  if (!vtg.mode_indicator.empty()) {
    printf("Mode: %s\n", vtg.mode_indicator.c_str());
  }
}

void PrintGll(const cpp_bus_driver::L76k::Gll& gll) {
  printf("------------GLL------------\n");
  if (!gll.update_flag) {
    return;
  }

  PrintLocation("GLL", gll.location);
  PrintUtc("GLL", gll.utc);
  if (!gll.location_status.empty()) {
    printf("Status: %s\n", gll.location_status.c_str());
  }
  if (!gll.mode_indicator.empty()) {
    printf("Mode: %s\n", gll.mode_indicator.c_str());
  }
}

void PrintTxt(const cpp_bus_driver::L76k::Txt& txt) {
  printf("------------TXT------------\n");
  if (!txt.update_flag) {
    return;
  }

  for (const auto& sentence : txt.sentences) {
    printf("TXT %u/%u id %u: %s\n",
        static_cast<unsigned int>(sentence.sentence_number),
        static_cast<unsigned int>(sentence.total_sentence_count),
        static_cast<unsigned int>(sentence.text_id), sentence.text.c_str());
  }
}

void PrintZda(const cpp_bus_driver::L76k::Zda& zda) {
  printf("------------ZDA------------\n");
  if (!zda.update_flag) {
    return;
  }

  PrintUtc("ZDA", zda.utc);
  PrintDate("ZDA", zda.date);
  printf("Local zone: %+d:%02d\n", static_cast<int>(zda.local_hour),
      static_cast<int>(zda.local_minute));
}

void PrintGnssInfo(const cpp_bus_driver::L76k::Info& info) {
  PrintRmc(info.rmc);
  PrintGga(info.gga);
  PrintGsv(info.gsv);
  PrintGsa(info.gsa);
  PrintVtg(info.vtg);
  PrintGll(info.gll);
  PrintTxt(info.txt);
  PrintZda(info.zda);
}

}  // namespace

extern "C" void app_main(void) {
  printf("Ciallo\n");

  auto& driver = lilygo_device_driver::TDisplayP4Driver::GetInstance();
  driver.Init();

  auto& l76k = driver.chip().l76k;

  while (true) {
    std::unique_ptr<uint8_t[]> buffer;
    uint32_t buffer_length = 0;

    if (l76k->GetInfoData(buffer, &buffer_length)) {
      printf("---begin---\n%.*s\n---end---\n", static_cast<int>(buffer_length),
          reinterpret_cast<const char*>(buffer.get()));

      cpp_bus_driver::L76k::Info info;
      if (l76k->ParseInfo(buffer.get(), buffer_length, info)) {
        PrintGnssInfo(info);
      } else {
        printf("GNSS parse failed\n");
      }
    }

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
