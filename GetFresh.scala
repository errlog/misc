import java.util.Date
import java.io._
import scala.io._
import scala.collection._
import sys.process._

def DoStuff()
{
  val url = "http://www.reddit.com/r/all/new.xml?sort=new"
  val xmlItems = (scala.xml.XML.loadString(Source.fromURL(url,"UTF-8").mkString) \\ "item")
  val bunch = for (n <- xmlItems) yield ((n\\"title").text, new Date((n\\"pubDate").text), (n\\"link").text)
  for ((a,b,c) <- bunch) { b.setHours(0); b.setMinutes(0); b.setSeconds(0); }
  val combined = bunch.toList

  var today = new java.util.Date()
  today.setHours(0);
  today.setMinutes(0);
  today.setSeconds(0);
  
  val checkSameData: ((String, java.util.Date, String)) => Boolean = { case(a,b,c) => b == today }
  combined.filter(checkSameData);
  
  var output:String = ""
  output += "<html><body>"
  for(i <- combined) {
      output +=i._1 + "<br/>";
      output += "<a href=\"" + i._3 +  "\">" + i._3 + "</a>" + "<br/><br/>"
  }
  output += "</body></html>";
  var f = new PrintWriter(new File("foo.html"));
  f.write(output);
  f.close()
}
DoStuff()