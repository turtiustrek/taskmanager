using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Drawing;
using System.IO;
namespace Converter
{
    class Program
    {

        static long map(long x, long in_min, long in_max, long out_min, long out_max)
        {
            return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
        }
        static void Main(string[] args)
        {
            String path = @"..\..\..\..\..\Video\frame\";
            StringBuilder sb = new StringBuilder("\n");
           sb.Append("int test[] = {\n");
            int fileCount = Directory.GetFiles(path).Length;
           // int ifd = 0;
            for (int index = 1; index <= fileCount; index++)
            {
                Bitmap bm = (Bitmap)Bitmap.FromFile(String.Format(@"{0}{1:000000}.bmp",path, index));


                for (int y = 0; y < bm.Height; y++)
                {
                    for (int x = 0; x < bm.Width; x++)
                    {
                       // int value = (bm.GetPixel(x, y).R + bm.GetPixel(x, y).G + bm.GetPixel(x, y).B) / 3;
                        sb.Append(map(bm.GetPixel(x, y).R, 255,0,0,100) + ",\n ");
                      //  ifd = ifd + 1;
                        //Console.WriteLine(RVal);
                    }
                }
               // Console.WriteLine(ifd);
            }
            sb.Append("\n};\n");
            File.WriteAllText(@"..\..\..\..\..\Dll1\TaskManager\video.h", sb.ToString());
        }
    }
}
