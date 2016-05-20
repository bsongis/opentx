---- #########################################################################
---- #                                                                       #
---- # Telemetry Widget script for FrSky Horus                               #
---- # Copyright (C) OpenTX                                                  #
-----#                                                                       #
---- # License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html               #
---- #                                                                       #
---- # This program is free software; you can redistribute it and/or modify  #
---- # it under the terms of the GNU General Public License version 2 as     #
---- # published by the Free Software Foundation.                            #
---- #                                                                       #
---- # This program is distributed in the hope that it will be useful        #
---- # but WITHOUT ANY WARRANTY; without even the implied warranty of        #
---- # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
---- # GNU General Public License for more details.                          #
---- #                                                                       #
---- #########################################################################


local options = {
  { "Sensor", SOURCE, 1 }, 
  { "Color", COLOR, WHITE }
}

local function create(zone, options)
  local myZone  = { zone=zone, options=options, counter=0 }
  histCellData = {}
  return myZone
end

local function isEmpty(self)
  for _, _ in pairs(self) do
    return false
  end
  return true
end

--- This function returns a table with cels values
local function getCels(sensor)
  liveCellData = getValue(sensor)
  if type(liveCellData) == "table" then
    if isEmpty(histCellData) then
      for k, v in pairs(liveCellData) do
        histCellData[k] = v
      end
    end
    if #histCellData ~= #liveCellData then -- this is necessary for simu
      for k, v in pairs(liveCellData) do
        histCellData[k] = v
      end
    end
    for k, v in pairs(liveCellData) do
      if v < histCellData[k] then histCellData[k] = v end
    end
  end
  return liveCellData
end

--- This function returns the number of cels
local function getCellCount(cellData)
  return #cellData
end

--- This function returns the value of the highest of all cels
local function getCellMax(cellData)
  cellMax = 0
  if type(cellData) == "table" then
    for k, v in pairs(cellData) do
      if v > cellMax then cellMax = v end
    end
  end
  return cellMax
end

--- This function returns the value of the lowest of all cels
local function getCellMin(cellData)
  cellMin = 5
  if type(cellData) == "table" then
    for k, v in pairs(cellData) do
      if v < cellMin then cellMin = v end
    end
  end
  return cellMin
end

--- This function return the percentage remaining in a single Lipo cel
local function getCellPercent(cellValue)
  --## Data gathered from commercial lipo sensors
  local myArrayPercentList =
{{3, 0}, {3.093, 1}, {3.196, 2}, {3.301, 3}, {3.401, 4}, {3.477, 5}, {3.544, 6}, {3.601, 7}, {3.637, 8}, {3.664, 9}, {3.679, 10}, {3.683, 11}, {3.689, 12}, {3.692, 13}, {3.705, 14}, {3.71, 15}, {3.713, 16}, {3.715, 17}, {3.72, 18}, {3.731, 19}, {3.735, 20}, {3.744, 21}, {3.753, 22}, {3.756, 23}, {3.758, 24}, {3.762, 25}, {3.767, 26}, {3.774, 27}, {3.78, 28}, {3.783, 29}, {3.786, 30}, {3.789, 31}, {3.794, 32}, {3.797, 33}, {3.8, 34}, {3.802, 35}, {3.805, 36}, {3.808, 37}, {3.811, 38}, {3.815, 39}, {3.818, 40}, {3.822, 41}, {3.825, 42}, {3.829, 43}, {3.833, 44}, {3.836, 45}, {3.84, 46}, {3.843, 47}, {3.847, 48}, {3.85, 49}, {3.854, 50}, {3.857, 51}, {3.86, 52}, {3.863, 53}, {3.866, 54}, {3.87, 55}, {3.874, 56}, {3.879, 57}, {3.888, 58}, {3.893, 59}, {3.897, 60}, {3.902, 61}, {3.906, 62}, {3.911, 63}, {3.918, 64}, {3.923, 65}, {3.928, 66}, {3.939, 67}, {3.943, 68}, {3.949, 69}, {3.955, 70}, {3.961, 71}, {3.968, 72}, {3.974, 73}, {3.981, 74}, {3.987, 75}, {3.994, 76}, {4.001, 77}, {4.007, 78}, {4.014, 79}, {4.021, 80}, {4.029, 81}, {4.036, 82}, {4.044, 83}, {4.052, 84}, {4.062, 85}, {4.074, 86}, {4.085, 87}, {4.095, 88}, {4.105, 89}, {4.111, 90}, {4.116, 91}, {4.12, 92}, {4.125, 93}, {4.129, 94}, {4.135, 95}, {4.145, 96}, {4.176, 97}, {4.179, 98}, {4.193, 99}, {4.2, 100}}

  for i, v in ipairs( myArrayPercentList ) do
    if v[ 1 ] >= cellValue then
      result =  v[ 2 ]
      break
     end
  end
  return result
end

--- This function parse each individual cell and return the sum of all cels
local function getCellSum(cellData)
    cellSum = 0
  if type(cellData) == "table" then
    for k, v in pairs(cellData) do cellSum = cellSum + v end
  end
  return cellSum
end

--- This function returns the average of all cels
local function getCellAvg(cellData)
  if type(cellData) == "table" then
    return getCellSum(cellData)/getCellCount(cellData)
  end
end

-- This function returns green at 100%, red bellow 30% and graduate in betwwen
local function getPercentColor(cpercent)
  if cpercent < 30 then
    return lcd.RGB(0xff, 0, 0)
  else
    g = math.floor(0xdf * cpercent / 100)
    r = 0xdf - g
    return lcd.RGB(r, g, 0)
  end
end

-- This function returns green at gvalue, red at rvalue and graduate in betwwen
local function getRangeColor(value, gvalue, rvalue)
  if gvalue > rvalue and not range==0 then
    range = gvalue - rvalue
    if value > gvalue then return lcd.RGB(0, 0xdf, 0) end
    if value < rvalue then return lcd.RGB(0xdf, 0, 0) end
    g = math.floor(0xdf * (value-rvalue) / range)
    r = 0xdf - g
    return lcd.RGB(r, g, 0)
  else
    range = rvalue - gvalue
    if value > gvalue then return lcd.RGB(0, 0xdf, 0) end
    if value < rvalue then return lcd.RGB(0xdf, 0, 0) end
    r = math.floor(0xdf * (value-gvalue) / range)
    g = 0xdf - r
    return lcd.RGB(r, g, 0)
  end
end

--- Size is 160x30
local function zoneSmall(zone)
  local myBatt = {["x"]=0, ["y"]=0, ["w"]=75, ["h"]=28, ["segments_w"]=15, ["color"]=WHITE, ["cath_w"]=6, ["cath_h"]=20}
  local mySensor = getCels(zone.options.Sensor)

  lcd.setColor(TEXT_COLOR, zone.options.Color)
  if type(mySensor) == "table" then
    local myString = tostring(getCellSum(mySensor)).."V ("..getCellCount(mySensor).."S)"
    local percent = getCellPercent(getCellAvg(mySensor))
    lcd.drawText(zone.zone.x + zone.zone.w, zone.zone.y + 22, myString, RIGHT + SMLSIZE + TEXT_COLOR)
    lcd.drawText(zone.zone.x + zone.zone.w, zone.zone.y, percent.."%", RIGHT + MIDSIZE + TEXT_COLOR)
    -- fils batt
    lcd.setColor(CUSTOM_COLOR, getPercentColor(percent))
    lcd.drawGauge(zone.zone.x+2, zone.zone.y+2, 75, zone.zone.h - 4, percent, 100, CUSTOM_COLOR)
    -- draws bat
    lcd.setColor(CUSTOM_COLOR, WHITE)
    lcd.drawRectangle(zone.zone.x + myBatt.x, zone.zone.y + myBatt.y, myBatt.w, myBatt.h, CUSTOM_COLOR, 2)
    lcd.drawFilledRectangle(zone.zone.x + myBatt.x + myBatt.w, zone.zone.y + myBatt.h/2 - myBatt.cath_h/2, myBatt.cath_w, myBatt.cath_h, CUSTOM_COLOR)
    for i=1, myBatt.w - myBatt.segments_w, myBatt.segments_w do
      lcd.drawRectangle(zone.zone.x + myBatt.x + i, zone.zone.y + myBatt.y, myBatt.segments_w, myBatt.h, CUSTOM_COLOR, 1)
    end
  else
    lcd.drawText(zone.zone.x, zone.zone.y+10, "No Cels sensor data", LEFT + SMLSIZE + INVERS + TEXT_COLOR)
  end
  return
end

--- Size is 180x70
local function zoneMedium(zone)
  local myBatt = {["x"]=0, ["y"]=0, ["w"]=75, ["h"]=32, ["segments_w"]=15, ["color"]=WHITE, ["cath_w"]=6, ["cath_h"]=20}
  local mySensor = getCels(zone.options.Sensor)

  lcd.setColor(TEXT_COLOR, zone.options.Color)
  if type(mySensor) == "table" then
    local percent = getCellPercent(getCellAvg(mySensor))
    lcd.drawText(zone.zone.x+102, zone.zone.y, percent.."%", LEFT + DBLSIZE + TEXT_COLOR)
    -- fils batt
    lcd.setColor(CUSTOM_COLOR, getPercentColor(percent))
    lcd.drawGauge(zone.zone.x + myBatt.x +myBatt.cath_w, zone.zone.y + myBatt.y, myBatt.w, myBatt.h, percent, 100, CUSTOM_COLOR)
    -- draw cells
    local pos = {{x=2, y=38}, {x=60, y=38}, {x=118, y=38}, {x=2, y=57}, {x=60, y=57}, {x=118, y=57}}
    for i=1, getCellCount(mySensor), 1 do
      lcd.setColor(CUSTOM_COLOR, getRangeColor(mySensor[i], getCellMax(mySensor), getCellMax(mySensor) - 0.2))
      lcd.drawFilledRectangle(zone.zone.x + pos[i].x, zone.zone.y + pos[i].y, 58, 20, CUSTOM_COLOR)
      lcd.drawText(zone.zone.x + pos[i].x+10, zone.zone.y + pos[i].y, string.format("%.2f", mySensor[i]))
      lcd.drawRectangle(zone.zone.x + pos[i].x, zone.zone.y + pos[i].y, 59, 20)
    end
  else
    lcd.drawText(zone.zone.x, zone.zone.y+35, "No Cels sensor data", LEFT + SMLSIZE + INVERS + TEXT_COLOR)
  end
  -- draws bat
  lcd.setColor(CUSTOM_COLOR, WHITE)
  lcd.drawRectangle(zone.zone.x + myBatt.x , zone.zone.y + myBatt.y, myBatt.w, myBatt.h, CUSTOM_COLOR, 2)
  lcd.drawFilledRectangle(zone.zone.x + myBatt.x + myBatt.w, zone.zone.y + myBatt.h/2 - myBatt.cath_h/2, myBatt.cath_w, myBatt.cath_h, CUSTOM_COLOR)
  for i=1, myBatt.w - myBatt.segments_w, myBatt.segments_w do
    lcd.drawRectangle(zone.zone.x + myBatt.x + i, zone.zone.y + myBatt.y, myBatt.segments_w, myBatt.h, CUSTOM_COLOR, 1)
  end
  return
end

--- Size is 190x150
local function zoneLarge(zone)
  local myBatt = {["x"]=0, ["y"]=18, ["w"]=76, ["h"]=121, ["segments_h"]=30, ["color"]=WHITE, ["cath_w"]=30, ["cath_h"]=10}
  local mySensor = getCels(zone.options.Sensor)

  lcd.setColor(TEXT_COLOR, zone.options.Color)
  if type(mySensor) == "table" then
    local percent = getCellPercent(getCellAvg(mySensor))
    lcd.drawText(zone.zone.x+zone.zone.w, zone.zone.y, percent.."%", RIGHT + XXLSIZE + TEXT_COLOR)
    lcd.drawText(zone.zone.x+zone.zone.w, zone.zone.y+44, tostring(getCellSum(mySensor)).."V", RIGHT + MIDSIZE + TEXT_COLOR)
    lcd.drawText(zone.zone.x+zone.zone.w, zone.zone.y+65, getCellCount(mySensor).."S", RIGHT + MIDSIZE + TEXT_COLOR)
    lcd.setColor(CUSTOM_COLOR, getPercentColor(percent))
    -- fils batt
    lcd.setColor(CUSTOM_COLOR, getPercentColor(percent))
    lcd.drawFilledRectangle(zone.zone.x + myBatt.x, zone.zone.y + myBatt.y + myBatt.h + myBatt.cath_h - math.floor(percent/100 * myBatt.h), myBatt.w, math.floor(percent/100 * myBatt.h), CUSTOM_COLOR)
    -- draw cells
    local pos = {{x=80, y=90}, {x=138, y=90}, {x=80, y=109}, {x=138, y=109}, {x=80, y=128}, {x=138, y=128}}
    for i=1, getCellCount(mySensor), 1 do
      lcd.setColor(CUSTOM_COLOR, getRangeColor(mySensor[i], getCellMax(mySensor), getCellMax(mySensor) - 0.2))
      lcd.drawFilledRectangle(zone.zone.x + pos[i].x, zone.zone.y + pos[i].y, 58, 20, CUSTOM_COLOR)
      lcd.drawText(zone.zone.x + pos[i].x+10, zone.zone.y + pos[i].y, string.format("%.2f", mySensor[i]))
      lcd.drawRectangle(zone.zone.x + pos[i].x, zone.zone.y + pos[i].y, 59, 20)
    end
  else
    lcd.drawText(zone.zone.x+5, zone.zone.y, "No Cels sensor data", LEFT + SMLSIZE + INVERS + TEXT_COLOR)
  end
  -- draws bat
  lcd.setColor(CUSTOM_COLOR, WHITE)
  lcd.drawRectangle(zone.zone.x + myBatt.x, zone.zone.y + myBatt.y + myBatt.cath_h , myBatt.w, myBatt.h, CUSTOM_COLOR, 2)
  lcd.drawFilledRectangle(zone.zone.x + myBatt.x + myBatt.w/2 - myBatt.cath_w/2, zone.zone.y + myBatt.y, myBatt.cath_w, myBatt.cath_h, CUSTOM_COLOR)
  for i=1, myBatt.h - myBatt.segments_h, myBatt.segments_h do
    lcd.drawRectangle(zone.zone.x + myBatt.x, zone.zone.y + myBatt.y + myBatt.cath_h + i, myBatt.w, myBatt.segments_h, CUSTOM_COLOR, 1)
  end
  return
end

--- Size is 390x170
local function zoneXLarge(zone)
  local myBatt = {["x"]=10, ["y"]=20, ["w"]=80, ["h"]=121, ["segments_h"]=30, ["color"]=WHITE, ["cath_w"]=30, ["cath_h"]=10}
  local mySensor = getCels(zone.options.Sensor)

  lcd.setColor(TEXT_COLOR, zone.options.Color)
  if type(mySensor) == "table" then
    local percent = getCellPercent(getCellAvg(mySensor))
    -- fils batt
    lcd.setColor(CUSTOM_COLOR, getPercentColor(percent))
    lcd.drawFilledRectangle(zone.zone.x + myBatt.x, zone.zone.y + myBatt.y + myBatt.h + myBatt.cath_h - math.floor(percent/100 * myBatt.h), myBatt.w, math.floor(percent/100 * myBatt.h), CUSTOM_COLOR)
    local percent = getCellPercent(getCellAvg(mySensor))
    -- draw right text section
    lcd.drawText(zone.zone.x+zone.zone.w, zone.zone.y + myBatt.y, percent.."%", RIGHT + XXLSIZE + TEXT_COLOR)
    lcd.drawText(zone.zone.x+zone.zone.w, zone.zone.y + myBatt.y + 60, tostring(getCellSum(mySensor)).."V", RIGHT + MIDSIZE + TEXT_COLOR)
    lcd.drawText(zone.zone.x+zone.zone.w, zone.zone.y + myBatt.y + 95, getCellCount(mySensor).."S", RIGHT + MIDSIZE + TEXT_COLOR)
    -- draw cells
    local pos = {{x=110, y=38}, {x=160, y=38}, {x=210, y=38}, {x=110, y=57}, {x=160, y=57}, {x=210, y=57}}
    for i=1, getCellCount(mySensor), 1 do
      lcd.setColor(CUSTOM_COLOR, getRangeColor(mySensor[i], getCellMax(mySensor), getCellMax(mySensor) - 0.2))
      lcd.drawFilledRectangle(zone.zone.x + pos[i].x, zone.zone.y + pos[i].y, 58, 20, CUSTOM_COLOR)
      lcd.drawText(zone.zone.x + pos[i].x+10, zone.zone.y + pos[i].y, string.format("%.2f", mySensor[i]))
      lcd.drawRectangle(zone.zone.x + pos[i].x, zone.zone.y + pos[i].y, 59, 20)
    end
    -- draw cells for lowest cells
    local pos = {{x=110, y=110}, {x=160, y=110}, {x=210, y=110}, {x=110, y=129}, {x=160, y=129}, {x=210, y=129}}
    for i=1, getCellCount(mySensor), 1 do
      lcd.setColor(CUSTOM_COLOR, getRangeColor(histCellData[i], mySensor[i], mySensor[i] - 0.3))
      lcd.drawFilledRectangle(zone.zone.x + pos[i].x, zone.zone.y + pos[i].y, 58, 20, CUSTOM_COLOR)
      lcd.drawText(zone.zone.x + pos[i].x+10, zone.zone.y + pos[i].y, string.format("%.2f", histCellData[i]))
      lcd.drawRectangle(zone.zone.x + pos[i].x, zone.zone.y + pos[i].y, 59, 20)
    end
  else
    lcd.drawText(zone.zone.x+5, zone.zone.y, "No Cels sensor data", LEFT + SMLSIZE + INVERS + TEXT_COLOR)
  end
  -- draws bat
  lcd.setColor(CUSTOM_COLOR, WHITE)
  lcd.drawRectangle(zone.zone.x + myBatt.x, zone.zone.y + myBatt.y + myBatt.cath_h , myBatt.w, myBatt.h, CUSTOM_COLOR, 2)
  lcd.drawFilledRectangle(zone.zone.x + myBatt.x + myBatt.w/2 - myBatt.cath_w/2, zone.zone.y + myBatt.y, myBatt.cath_w, myBatt.cath_h, CUSTOM_COLOR)
  for i=1, myBatt.h - myBatt.segments_h, myBatt.segments_h do
    lcd.drawRectangle(zone.zone.x + myBatt.x, zone.zone.y + myBatt.y + myBatt.cath_h + i, myBatt.w, myBatt.segments_h, CUSTOM_COLOR, 1)
  end
  -- draw middle rectangles
  lcd.drawRectangle(zone.zone.x + 110, zone.zone.y + 38, 160, 40, CUSTOM_COLOR, 1)
  lcd.drawText(zone.zone.x + 270, zone.zone.y + 21, "Live data", RIGHT + SMLSIZE + INVERS + TEXT_COLOR)
  lcd.drawRectangle(zone.zone.x + 110, zone.zone.y + 110, 160, 40, CUSTOM_COLOR, 1)
  lcd.drawText(zone.zone.x + 270, zone.zone.y + 93, "Lowest data", RIGHT + SMLSIZE + INVERS + TEXT_COLOR)

  return
end

function refresh(myZone)
  if myZone.options.Sensor == 1 then
    lcd.drawText(myZone.zone.x+2, myZone.zone.y+2, "BattCheck not configured", LEFT + SMLSIZE + INVERS + TEXT_COLOR)
    return
  end
  if myZone.zone.w  > 380 and myZone.zone.h > 165 then zoneXLarge(myZone)
  elseif myZone.zone.w  > 180 and myZone.zone.h > 145  then zoneLarge(myZone)
  elseif myZone.zone.w  > 170 and myZone.zone.h > 65 then zoneMedium(myZone)
  elseif myZone.zone.w  > 150 and myZone.zone.h > 28 then zoneSmall(myZone)
  end
end

return { name="BattCheck", options=options, create=create, refresh=refresh }
