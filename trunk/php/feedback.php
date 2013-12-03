<?php
// Note from Kahunapule 18 Nov 2013: Please don't use this script to send email directly to JarMail addresses.
// Spam filters will not allow delivery. Instead, choose one or more addresses in the adapt-it.org domain and
// either check mail directly on those additional accounts or use RPOP to get the mail.

$recipient = "support@adapt-it.org";
//$recipient = "developers@adapt-it.org";
//$recipient = "bill_martin@adapt-it.org";
//$recipient = "whmartin@sbcglobal.net";

if (empty($_POST)) {

    // We only accpet POSTs
    header('HTTP/1.0 403 Forbidden');
    exit;

} else {

    $subject = "[Adapt It ";
    $subject .= $_POST['reporttype']."] ";
    $subject .= $_POST['emailsubject'];
    
    $random_hash = md5(date('r', time())); 
    $notify_log = "Log is Attached:";
    $attach_log = $_POST['attachlog'];
    $notify_doc = "Packed Document Is Attached:";
    $attach_doc = $_POST['attachdoc'];
    
    $headers = "From: ";
    $headers .= "Adapt It <robot@adapt-it.org>\r\n";
    $headers .= "Reply-To: ";
    $headers .= $_POST['senderemailaddr']."\r\n";
    $headers .= "MIME-Version: 1.0\r\n";
    
    if ($attach_log == $notify_log || $attach_doc == $notify_doc)
    {
        // There is at least one attachment so use multipart/mixed and MIME boundary
        $headers .= "Content-Type: multipart/mixed; boundary=\"Adapt It--{$random_hash}\"\r\n\r\n"; 
        // Note: The boundary string behavior is described in RFC1341 part 7.2.1
        // If boundary="simple boundary", then each part should begin with "--simple boundary\r\n"
        // with the last "Content-... declarations line ending with double CRLFs \r\n\r\n. The final
        // boundary is of the form "--simple boundary--\r\n"
        $message .= "This is a multi-part message in MIME format.\r\n";
        $message .= "--Adapt It--{$random_hash}\r\n";
        $message .= "Content-Type: text/plain; charset=\"iso-8859-1\"; format=flowed\r\n";
        $message .= "Content-Transfer-Encoding: 7bit\r\n\r\n";
    }
    
    // build the message body
    $message .= "Submitted at ".date("F j, Y, g:i a")."\r\n";
    $message .= "To: Adapt It Developers <support@adapt-it.org>\r\n";
    $message .= "Name of Sender: ";
    $message .= $_POST['sendername']."\r\n";
    $message .= "Email address of Sender: ";
    $message .= $_POST['senderemailaddr']."\r\n";
    if ($attach_log == $notify_log)
    {
        // Add plain text note in email body that "Log Is Attached:"
        $message .= "      ".$notify_log."\r\n";
    }
    if ($attach_doc == $notify_doc)
    {
        // Add plain text note in email body that "Packed Document Is Attached:"
        $message .= "      ".$notify_doc."\r\n";
    }
    
    $message .= "\r\n".$_POST['reporttype'].":\r\n";
    $message .= $_POST['emailbody']."\r\n\r\n";
    
    $message .= "System Information:\r\n";
    $message .= $_POST['sysinfo']."\r\n\r\n";;
    
    if ($attach_log == $notify_log)
    {
        // the userlog is to be attached so build it
        $message .= "--Adapt It--{$random_hash}\r\n";
        $message .= "Content-Type: application/octet-stream; name=\"userlog.zip\"\r\n"; 
        $message .= "Content-Transfer-Encoding: base64\r\n";
        $message .= "Content-Disposition: attachment; filename=\"userlog.zip\"\r\n\r\n";
        // Note: userlog below was already encoded for base64 by Adapt It before posting
        $attachment_log = chunk_split($_POST['userlog']);
        $message .= $attachment_log;
    }
    
    if ($attach_doc == $notify_doc)
    {
        // the packed document is to be attached so build it
        $message .= "--Adapt It--{$random_hash}\r\n";
        $message .= "Content-Type: application/octet-stream; name=\"packeddoc.aip\"\r\n"; 
        $message .= "Content-Transfer-Encoding: base64\r\n";
        $message .= "Content-Disposition: attachment; filename=\"packeddoc.aip\"\r\n\r\n";
        // Note: packdoc below was already encoded for base64 by Adapt It before posting
        $attachment_doc = chunk_split($_POST['packdoc']);
        $message .= $attachment_doc;
    }
    
    if ($attach_log == $notify_log || $attach_doc == $notify_doc)
    {
      // Final boundary, but only if there were attachments
      $message .= "--Adapt It--{$random_hash}--\r\n";
    }

    // Send message to email address
    $sent = mail($recipient, $subject, $message, $headers);

    if ($sent) {
?>

        <html>
        <body>

        Got POST and sent email:
        <pre><? echo $message; ?></pre>

        </body>
        </html>

<?php
    } else {
        // Return an error
        header('HTTP/1.0 500 Internal Server Error', true, 500);
        exit;
    }
}
?>

