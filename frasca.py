#import utm
import math
import geopy
import geopy.distance
from geographiclib.geodesic import Geodesic


FRASCA_WSG86_500POINT_LAT=64.93166666666667
FRASCA_WSG86_500POINT_LON=25.359166666666667

global point0
point0=geopy.Point(FRASCA_WSG86_500POINT_LAT,FRASCA_WSG86_500POINT_LON)

def from_frasca(x,y, force_point0=False):
        global point0
        dx=x-500.0
        dy=y-500.0
        dc=math.sqrt(pow(dy,2)+pow(dx,2))
        alpha=0 if dy == 0.0 else math.atan(abs(dx)/abs(dy))
        heading=(alpha if dy>0 and dx>=0
                         else
                         (math.pi-alpha if dy<=0 and dx>0
                          else
                          (alpha+math.pi if dy<0 and dx<=0
                           else
                           (2*math.pi-alpha if dy>=0 and dx<0
                            else -1))))

        dist = geopy.distance.geodesic(nautical=dc)
        point1 = dist.destination(point=(force_point0 if force_point0 else point0),bearing=heading*(180.0/math.pi))
        return point1

def to_frasca(lat,lon):
        global point0
        diff=9999999.99
        point1 = geopy.Point(lat,lon)
        pointX = point0
        r_p = (500,500)
        max_iter = 20
        while(diff>1. and max_iter > 0):
                tmp = Geodesic.WGS84.Inverse(pointX.latitude, pointX.longitude, point1.latitude, point1.longitude)
                dx = math.sin(math.radians(tmp["azi1"]))*tmp["s12"]/1000/1.852
                dy = math.cos(math.radians(tmp["azi1"]))*tmp["s12"]/1000/1.852
                r_p = (r_p[0]+dx, r_p[1]+dy)
                pointX = from_frasca(r_p[0],r_p[1])
                diff = tmp["s12"]
                max_iter = max_iter-1
        return r_p
                
                             

# Backup: utm functions, these work quite nicely when working within same zone
'''
point0 = utm.from_latlon(FRASCA_WSG86_500POINT_LAT,FRASCA_WSG86_500POINT_LON)


def to_frasca(lat, lon):
        global point0
        point1 = utm.from_latlon(lat, lon, force_zone_number=point0[2])
        x = 500-(point0[0]-point1[0])/1000/1.852
        y = 500-(point0[1]-point1[1])/1000/1.852
        return (x, y)

def from_frasca(dx, dy):
        global point0
        x = point0[0]-(500-dx)*1.852*1000
        y = point0[1]-(500-dy)*1.852*1000
        zone = point0[2]
        if x<100000:
                #zone_diff = math.floor(x/800000)
                #x = x - zone_diff * 800000
                #zone = zone + zone_diff
                x=100000
        if x>800000:
                x=800000
        point1 = utm.to_latlon(x,y,zone, northern=True)
        return point1
'''
