using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using Paratext;

namespace rdwrtp7
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length != 5)
            {
                Console.WriteLine("Usage: rdwrtp7 -r|-w project book chapter|0 fileName");
                Environment.Exit(-1);
            }

            ScrTextCollection.Initialize();

            string operation = args[0];
            string project = args[1];
            string book = args[2];
            string chapter = args[3];
            string file = args[4];

            ScrText scr = ScrTextCollection.Get(project);
            if (scr == null)
            {
                Console.WriteLine("Error: unknown project");
                Environment.Exit(-1);
            }

            VerifyBook(book);
            VerifyChapter(chapter);

            VerseRef vref = new VerseRef(book, chapter, "1", scr.Versification);

            try
            {
                if (operation == "-r")
                    DoRead(scr, vref, file);
                else if (operation == "-w")
                    DoWrite(scr, vref, file);
                else
                {
                    Console.WriteLine("Error: unknown operation");
                    Environment.Exit(-1);
                }
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                Environment.Exit(-1);
            }
        }

        private static void VerifyChapter(string chapter)
        {
            int result;
            if (!int.TryParse(chapter, out result))
            {
                Console.WriteLine("Error: invalid chapter number");
                Environment.Exit(-1);
            }
        }

        private static void VerifyBook(string book)
        {
            if (!Canon.IsBookIdValid(book))
            {
                Console.WriteLine("Error: invalid book name");
                Environment.Exit(-1);
            }
        }

        private static void DoWrite(ScrText scr, VerseRef vref, string file)
        {
            string text;
            using (StreamReader reader = new StreamReader(file, Encoding.UTF8))
            {
                text = reader.ReadToEnd();
            }

            string chapter = (vref.Chapter == "0") ? null : vref.Chapter;

            scr.PutText(vref.Book, chapter, false, text, null);
        }

        private static void DoRead(ScrText scr, VerseRef vref, string file)
        {
            string text = scr.GetText(vref, vref.ChapterNum != 0, false);
            using (StreamWriter writer = new StreamWriter(file, false, Encoding.UTF8))
            {
                writer.Write(text);
            }
        }
    }
}
