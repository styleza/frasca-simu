import frasca
import geopy

def aip_transform(a):
	return "%s %sm %ss %s %s %sm %ss %s" % (a[0:2], a[2:4], a[4:6], a[7], a[8:11], a[11:13], a[13:15], a[15])

while(True):
        ll = input("coordinate: ")
        if not ll:
                break
        p = geopy.Point(aip_transform(ll))
        f = frasca.to_frasca(p.latitude, p.longitude)
        print("%f\t%f" % (f[0],f[1]))
		
