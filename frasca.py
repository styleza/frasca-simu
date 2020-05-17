import utm

FRASCA_WSG86_500POINT_LAT=64.931388
FRASCA_WSG86_500POINT_LON=25.375800
global point0
point0 = utm.from_latlon(FRASCA_WSG86_500POINT_LAT,FRASCA_WSG86_500POINT_LON)



def to_frasca(lat, lon):
        global point0
        point1 = utm.from_latlon(lat, lon, ) #point0[2], point0[3])
        return (500-(point0[0]-point1[0])/1000/1.852, 500-(point0[1]-point1[1])/1000/1.852)

def from_frasca(dx, dy):
        global point0
        x = point0[0]-(500-dx)*1.852*1000
        y = point0[1]-(500-dy)*1.852*1000
        print(x,y)
        point1 = utm.to_latlon(x,y,point0[2],point0[3])
        return point1
