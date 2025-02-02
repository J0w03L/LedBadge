﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;

namespace LedBadge
{
    class ImageProvider
    {
        public ImageProvider(Dispatcher dispatcher, LedBadgeLib.MessageQueue messageQueue)
        {
            Dispatcher = dispatcher;
            m_messageQueue = messageQueue;
        }

        public Dispatcher Dispatcher { get; set; }
        public bool Dither { get; set; }
        
        LedBadgeLib.MessageQueue m_messageQueue;

        public void SendImage(string path)
        {
            var img = new BitmapImage(new Uri(path, UriKind.Absolute));
            CheckImage(img);
        }

        void CheckImage(BitmapImage img)
        {
            if(img.IsDownloading)
            {
                Dispatcher.InvokeAsync(() => CheckImage(img), DispatcherPriority.Background);
            }
            else
            {
                var el = new System.Windows.Controls.Image()
                {
                    Source = img,
                    SnapsToDevicePixels = true,
                    UseLayoutRounding = true,
                    MinWidth = LedBadgeLib.BadgeCaps.Width,
                    MinHeight = LedBadgeLib.BadgeCaps.Height,
                    Width = img.Width * ((BitmapSource)img).DpiX / 96,
                    Stretch = Stretch.UniformToFill,
                };
                RenderOptions.SetBitmapScalingMode(img, BitmapScalingMode.NearestNeighbor);
                el.Measure(new Size(LedBadgeLib.BadgeCaps.Width, LedBadgeLib.BadgeCaps.Height));
                el.Arrange(new Rect(0, 0, LedBadgeLib.BadgeCaps.Width, LedBadgeLib.BadgeCaps.Height));

                m_messageQueue.Enqueue(new LedBadgeLib.MessageQueueItem(new LedBadgeLib.WpfVisual(el, dither : Dither)));
            };
        }
    }
}
