<?php

/**
 * @file
 * Page callbacks for signatures_queue module.
 */

/**
 * Callback for admin/config/system/signatures-queue/monitor.
 *
 * Provide basic dashboard for monitoring signatures queues.
 */
function signatures_queue_monitor_page() {
  return 'Hello world!';
}

/**
 * Callback for admin/config/system/petitions/signatures-queue.
 *
 * Provide basic overview of the Signatures Queue system.
 */
function signatures_queue_overview() {
  $rows = array();
  $queues = signatures_queue_get_queue_names();
  foreach ($queues as $queue_name => $description) {
    $rows[] = array($queue_name, $description);
  }
  $rows[] = array(t('NOTE'), t("Real queues are named something_queue. The tables listed above that don't end in the suffix \"_queue\" aren't actually queues. Conceptually they're part of the \"signatures queue\" in the sense that these are holding bins where submitted signatures go before they're done being processed and counted. But they're not DrupalQueue queues. They cannot be instantiated with SignaturesQueue::get()."));

  $output = theme('table', array(
    'header' => array(
      t('Queue name'),
      t('Description')),
    'rows' => $rows,
  ));

  return $output;
}

/**
 * Callback for thank-you.
 *
 * Users hit this page when they click a validation link in an email to confirm
 * their signature. The link will look like this:
 *
 *   example.com/thank-you?k=[secret-validation-key]&m=[md5-hash-of-key]&p=[petition-id]
 *
 * This page invokes the receive_signature_validation workflow which returns a
 * URL for a drupal_goto(), which will say "thank you" or provide and error
 * message to the user.
 */
function _signatures_queue_validation_page_callback() {
  // Get parameters submitted via URL.
  if (empty($_GET['k']) || empty($_GET['m']) || empty($_GET['p']) || empty($_GET['d']) || !is_numeric($_GET['d'])) {
    // Parameters are missing. Return error.
    drupal_not_found();
  }

  // Sanitize data coming in through URL. 
  $validation_key = check_plain($_GET['k']);
  $md5 = check_plain($_GET['m']);
  $petition_id = check_plain($_GET['p']);
  $validation_expiration = check_plain($_GET['d']);

  // Pass info from URL to signatures_queue_receive_signature_validation
  // workflow via $options array.
  $options = array(
    'secret_validation_key' => $validation_key,
    'md5' => $md5,
    'petition_id' => $petition_id,
    'validation_expiration' => $validation_expiration,
  );

  // Invoke the workflow signatures_queue_receive_signature_validation.
  // If this is successful, this should return a URL for a thank you page that
  // we will redirect the user to, to thank them and let them know their
  // signature will be counted.
  $response = signatures_queue_invoke_workflow('receive_signature_validation', $options);

  // Typically signatures_queue_invoke_workflow() returns boolean.
  // If the workflow fails (shunt is tripped), the response will be TRUE.
  // In this case, drupal_goto() apology page.
  if ($response == 500) {
    drupal_goto('validation-error', array(), 307);
  }
  elseif ($response == 'error') {
    drupal_not_found();
  }
  
  // If we get here, $response is a URL for a thank you page.
  drupal_goto($response, array(), 301);
}

/**
 * Callback for thank-you/%.
 */
function _signatures_queue_validated_page_callback($petition_id) {
  $petition = petitions_data_get_petition($petition_id);
  if (!empty($petition)) {
    // Load the petition via the API retrieve method.
    $title = $petition['title'];
    $output = "Thank you for signing '{$title}' Your signature has been counted.";
    return $output;
  }
  else {
    drupal_goto('petition-error', array(), 301);
  }
}

/**
 * Callback for validation-error.
 */
function _signatures_queue_validation_error_page_callback() {
  $output = "This service is temporarily unavailable. Please try again later.";
  return $output;
}

/**
 * Callback fo petition-error.
 */
function _signatures_queue_petition_error_page_callback() {
  $output = "Sorry, your petition has been removed, expired, or doesn't exist.";
  return $output;
}
