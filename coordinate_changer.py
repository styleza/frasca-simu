import frasca
import geopy

def aip_transform(a):
	return "%s %sm %ss %s %s %sm %ss %s" % (a[0:2], a[2:4], a[4:6], a[7], a[8:11], a[11:13], a[13:15], a[15])


while(True):
        ident = input("ident: ")
        if not ident:
                break
        try:
                coord = input("coordinate: ")
                p = geopy.Point(aip_transform(coord))
        except:
                print("invalid coordinate, format should be same as in aip for example '601942N 0245808E'")
                continue
        
        f = frasca.to_frasca(p.latitude, p.longitude)
        print("%s\tNORTH\t%f\tEAST\t%f" % (ident, f[1],f[0]))
		
