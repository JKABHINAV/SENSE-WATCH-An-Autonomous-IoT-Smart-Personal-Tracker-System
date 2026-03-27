function doGet(e) {
  var sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("TrackerData");
  
  // 1. ESP32 SENDING DATA (Writing)
  if (e.parameter.lat && e.parameter.lon) {
    var date = e.parameter.date || "N/A";
    var time = e.parameter.time || "N/A";
    var lat = e.parameter.lat;
    var lon = e.parameter.lon;
    var status = e.parameter.status || "NORMAL";
    
    sheet.appendRow([date, time, lat, lon, status]);
    return ContentService.createTextOutput("Success").setMimeType(ContentService.MimeType.TEXT);
  } 
  
  // 2. WEBSITE REQUESTING DATA (Reading)
  else {
    // RECTIFICATION: Using getDisplayValues() instead of getValues() 
    // This stops the 1899-12-29 and ISO timestamp issues.
    var rows = sheet.getDataRange().getDisplayValues(); 
    var data = [];
    
    for (var i = 1; i < rows.length; i++) {
      data.push({
        date: rows[i][0],    // Now sends "04-03-2026"
        time: rows[i][1],    // Now sends "21:04:34"
        lat: rows[i][2],
        lon: rows[i][3],
        status: rows[i][4]
      });
    }
    
    return ContentService.createTextOutput(JSON.stringify(data.reverse()))
      .setMimeType(ContentService.MimeType.JSON);
  }
}
