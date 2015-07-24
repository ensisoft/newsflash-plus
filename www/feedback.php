<?php
  include("common.php");
  include("database.php");
  include("credentials.php");

  $type     = sql_string($_REQUEST['type']);
  $name     = sql_string($_REQUEST['name']);
  $email    = sql_string($_REQUEST['email']);
  $country  = sql_string($_REQUEST['country']);
  $text     = sql_string($_REQUEST['text']);
  $version  = sql_string($_REQUEST['version']);
  $platform = sql_string($_REQUEST['platform']);
  $host     = sql_string(get_host_name());
  $tmpfile  = $_FILES['attachment']['tmp_name'];
  $filename = $_FILES['attachment']['name'];

  switch ($type) {
      case sql_string($TYPE_NEGATIVE_FEEDBACK): 
          $typename = "Feedback :(";
          break;
      case sql_string($TYPE_POSITIVE_FEEDBACK): 
          $typename = "Feedback :)";
          break;
      case sql_string($TYPE_NEUTRAL_FEEDBACK):  
          $typename = "Feedback :|";
          break;
      case sql_string($TYPE_BUG_REPORT):        
          $typename = "Bug report";
          break;
      case sql_string($TYPE_FEATURE_REQUEST):   
          $typename = "Feature request";
          break;
      case sql_string($TYPE_LICENSE_REQUEST):
          $typename = "License request";
          break;
  }  

  if (!strlen($typename))
      die($ERROR_QUERY_PARAMS . " type");
  if (!strlen($name))
      die($ERROR_QUERY_PARAMS . " name");
  if(!strlen($text))
      die($ERROR_QUERY_PARAMS . " text");

  if (sql_check_spam("feedback", $host))
      die($DIRTY_ROTTEN_SPAMMER);
  
  $insert = "INSERT INTO feedback " .
            "( type,   name, email,  host,  country,  text,  version,  platform) VALUES " .
            "($type, $name, $email, $host, $country, $text, $version, $platform)";
  mysql_query($insert, $db) or die($DATABASE_ERROR);
  $id = mysql_insert_id($db);
  mysql_close($db);

  // create and send email notification
  $subject = "[NEW FEEDBACK - ID: $id]";
  $text    = "Hello, new feedback was received\n\n" .
             "ID: $id\n" .
             "Type: $typename\n" .
             "Name: $name\n" .
             "Email: $email\n" . 
             "Country: $country\n" .
             "Host: $host\n" .
             "Version: $version\n" .
             "Platform: $platform\n\n" .
             "$text"; 

  if (strlen($email))
      $headers = "From: $email\r\nReply-To: $email\r\n";

  if (strlen($tmpfile))
  {
      //$mimetype    = mime_content_type($tmpfile);
      $mimetype    = shell_exec("file -bi " . $tmpfile);
      $random_hash = md5('r', time());
      $boundary    = "boundary-$random_hash";
      $attachment  = chunk_split(base64_encode(file_get_contents($tmpfile)));
      $body =  "This is a multi-part message in MIME format.\n\n" .
               "--$boundary\n" .
               "Content-Type: text/plain; charset=utf-8\n\n" .
               "$text\n" .
               "--$boundary\n" .
               "Content-Type: $mimetype; Name=\"$filename\"\n" .
               "Content-Transfer-Encoding: base64\n" .
               "Content-Disposition: attachment; filename=\"$filename\"\n\n" .
               "$attachment\n" .
               "--$boundary--" ;

      $headers .= "Content-Type: multipart/mixed; boundary=\"$boundary\"\n";
      $headers .= "Content-Transfer-Encoding: 7bit\n";
      $headers .= "MIME-Version: 1.0\n";
  }
  else
  {
      $headers .= "Content-Type: text/plain; charset=utf-8\r\n";
      $body = $text;
  }
  $mail_sent = @mail($MY_EMAIL, $subject, $body, $headers);
  if (!$mail_sent)
  {
      // todo: transactional semantics
  }
  echo $SUCCESS;
  
?>

