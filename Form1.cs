using odm.core;
using onvif.utils;
using utils;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Reactive;
using System.Reactive.Linq;
using System.Reactive.Disposables;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using onvif.services;
using System.Net;
using video.player;
using System.Threading;
using System.Diagnostics;
using System.Windows.Threading;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

namespace Winform {
    public partial class Form1 : Form, IPlaybackController { 
        readonly string preff = @"http://";
		readonly string postf = @"/onvif/device_service";

        HostedPlayer player;
        VideoBuffer videoBuff;
        IPlaybackSession playbackSession;

        object sync = new object();

        CompositeDisposable disposables = new CompositeDisposable();
        CompositeDisposable playerDisposables = new CompositeDisposable();

        Dispatcher disp;
        Image img;

        public Form1() {
			InitializeComponent();

            disp = Dispatcher.CurrentDispatcher;

			valueipaddr.Text = "192.168.2.136";
			valuename.Text = "";
			valuepass.Text = "";

			valueipaddr.TextChanged += (o, e) => {
				Validate();
			};
		}
		public string address { 
			get {
				return preff + valueipaddr.Text + postf;
			} 
		}
		void Validate() {
			Uri dummyUri;
            button1.Enabled = Uri.TryCreate(address, UriKind.Absolute, out dummyUri);
		}

        System.Net.NetworkCredential account = null;
        private void button1_Click(object sender, EventArgs e){
            var name = valuename.Text;
            var pass = valuepass.Text;
            Release();
            
            if (!string.IsNullOrEmpty(name)) {
                account = new System.Net.NetworkCredential(name, pass);
            }

            NvtSessionFactory factory = new NvtSessionFactory(account);

           disposables.Add(factory.CreateSession(new Uri[] { new Uri(address) })
               .ObserveOnCurrentDispatcher()
               .Subscribe(
               session => {
                   GetProfiles(session);
               }, err => {
                   errBox.Text = err.Message;
               }));
        }

        void GetProfiles(INvtSession session) {

            //Every video source can have it's own profiles
            disposables.Add(session.GetProfiles()
                .ObserveOnCurrentDispatcher()
                .Subscribe(
                profs=>{
                    var selectedProfile = profs.FirstOrDefault();
                    if (selectedProfile == null) {
                        //Device vave not any profiles
                        errBox.Text = "Profile is empty";
                    } else {
                        GetStreamUri(session, selectedProfile);
                    }
                }, err=>{
                    errBox.Text = err.Message;
                }));
        }

        void GetStreamUri(INvtSession session, onvif.services.Profile prof) {
            var srtSetup = new StreamSetup() {
                stream = StreamType.rtpUnicast,
                transport = new Transport() {
                    protocol = TransportProtocol.udp
                }
            };

            //Get stream uri for selected profile
            disposables.Add(session.GetStreamUri(srtSetup, prof.token)
                .ObserveOnCurrentDispatcher()
                .Subscribe(
                muri => {
                    Size videosize = new Size(prof.videoEncoderConfiguration.resolution.width, prof.videoEncoderConfiguration.resolution.height);

                    InitPlayer(muri.uri.ToString(), account, videosize);
                }, err => {
                    errBox.Text = err.Message;
                }));
        }

        public void InitPlayer(string videoUri, NetworkCredential account, Size sz = default(Size)) {
            player = new HostedPlayer();
            playerDisposables.Add(player);

            if (sz != default(Size))
                videoBuff = new VideoBuffer((int)sz.Width, (int)sz.Height, PixFrmt.bgra32);
            else {
                videoBuff = new VideoBuffer(720, 576, PixFrmt.bgra32);
            }
            player.SetVideoBuffer(videoBuff);

            MediaStreamInfo.Transport transp = MediaStreamInfo.Transport.Udp;
            MediaStreamInfo mstrInfo = null;
            if (account != null) {
                mstrInfo = new MediaStreamInfo(videoUri, transp, new UserNameToken(account.UserName, account.Password));//, transp, null);
            } else {
                mstrInfo = new MediaStreamInfo(videoUri, transp);
            }

            playerDisposables.Add(
                player.Play(mstrInfo, this)
            );
            InitPlayback(videoBuff, true);

            //{ // playing controls
            //    btnPause.Click += play;
            //    btnResume.Click += play;
            //}
        }
        void Restart() { 
        }
        //private WriteableBitmap PrepareForRendering(VideoBuffer videoBuffer) {
        //    PixelFormat pixelFormat;
        //    if (videoBuffer.pixelFormat == PixFrmt.rgb24) {
        //        pixelFormat = PixelFormats.Rgb24;
        //    } else if (videoBuffer.pixelFormat == PixFrmt.bgra32) {
        //        pixelFormat = PixelFormats.Bgra32;
        //    } else if (videoBuffer.pixelFormat == PixFrmt.bgr24) {
        //        pixelFormat = PixelFormats.Bgr24;
        //    } else {
        //        pixelFormat = PixelFormats.Rgb24;
        //    }
        //    var bitmap = new WriteableBitmap(
        //        videoBuffer.width, videoBuffer.height,
        //        96, 96,
        //        pixelFormat, null
        //    );
        //    _imgVIew.Source = bitmap;
        //    return bitmap;
        //}
        void InitPlayback(VideoBuffer videoBuffer, bool isInitial) {
            if (videoBuffer == null) {
                dbg.Break();
                log.WriteError("video buffer is null");
            }

            TimeSpan renderinterval;
            try {
                int fps = 30;
                fps = (fps <= 0 || fps > 100) ? 100 : fps;
                renderinterval = TimeSpan.FromMilliseconds(1000 / fps);
            } catch {
                renderinterval = TimeSpan.FromMilliseconds(1000 / 30);
            }

            var cancellationTokenSource = new CancellationTokenSource();
            playerDisposables.Add(Disposable.Create(() => {
                cancellationTokenSource.Cancel();
            }));
            
            //var bitmap = PrepareForRendering(videoBuffer);
            img = new Bitmap(videoBuffer.width, videoBuffer.height);
            imageBox.Image = img;

            var cancellationToken = cancellationTokenSource.Token;

            var renderingTask = Task.Factory.StartNew(delegate {
                var statistics = PlaybackStatistics.Start(Restart, isInitial);
                using (videoBuffer.Lock()) {
                    try {
                        //start rendering loop
                        while (!cancellationToken.IsCancellationRequested) {
                            using (var processingEvent = new ManualResetEventSlim(false)) {
                                var dispOp = disp.BeginInvoke(delegate {
                                    using (Disposable.Create(() => processingEvent.Set())) {
                                        if (!cancellationToken.IsCancellationRequested) {
                                            //update statisitc info
                                            statistics.Update(videoBuffer);

                                            //render farme to screen
                                            //DrawFrame(bitmap, videoBuffer, statistics);
                                            DrawFrame(videoBuffer, statistics);
                                        }
                                    }
                                });
                                processingEvent.Wait(cancellationToken);
                            }
                            cancellationToken.WaitHandle.WaitOne(renderinterval);
                        }
                    } catch (OperationCanceledException) {
                        //swallow exception
                    } catch (Exception error) {
                        dbg.Break();
                        log.WriteError(error);
                    } finally {
                    }
                }
            }, cancellationToken);
        }

        [DllImport("kernel32.dll", EntryPoint = "CopyMemory", SetLastError = false)]
        public static extern void CopyMemory(IntPtr dest, IntPtr src, int count);
        private void DrawFrame(VideoBuffer videoBuffer, PlaybackStatistics statistics) {
            Bitmap bmp = img as Bitmap;
            BitmapData bd = null;
            try {
                bd = bmp.LockBits(new System.Drawing.Rectangle(0, 0, bmp.Width, bmp.Height), System.Drawing.Imaging.ImageLockMode.WriteOnly, System.Drawing.Imaging.PixelFormat.Format32bppArgb);//bgra32

                using (var md = videoBuffer.Lock()) {

                    CopyMemory(bd.Scan0, md.value.scan0Ptr, videoBuff.stride * videoBuff.height);
                    
                    //bitmap.WritePixels(
                    //    new Int32Rect(0, 0, videoBuffer.width, videoBuffer.height),
                    //    md.value.scan0Ptr, videoBuffer.size, videoBuffer.stride,
                    //    0, 0
                    //);
                }

            } catch (Exception err) {
                errBox.Text = err.Message;
            } finally {
                bmp.UnlockBits(bd);
            }
            imageBox.Image = bmp;
        }

        //private void DrawFrame(WriteableBitmap bitmap, VideoBuffer videoBuffer, PlaybackStatistics statistics) {
            //bitmap.Lock();
            //try {
            //    using (var md = videoBuffer.Lock()) {
            //        bitmap.WritePixels(
            //            new Int32Rect(0, 0, videoBuffer.width, videoBuffer.height),
            //            md.value.scan0Ptr, videoBuffer.size, videoBuffer.stride,
            //            0, 0
            //        );
            //    }
            //} finally {
            //    bitmap.Unlock();
            //}
            //fpsCaption.Text = statistics.avgRenderingFps.ToString("F1");
            //noSignalPanel.Visibility =
            //    statistics.isNoSignal ? Visibility.Visible : Visibility.Hidden;
        //}

        void Release() {
            disposables.Dispose();
            disposables = new CompositeDisposable();

            playerDisposables.Dispose();
            playerDisposables = new CompositeDisposable();
        }

        public bool Initialized(IPlaybackSession playbackSession) {
            this.playbackSession = playbackSession;
            return true;
        }
        public void Shutdown() {
        }

        private class PlaybackStatistics {
            private PlaybackStatistics(Action Restart, bool isInitial) {
                signal = 0;
                this.Restart = Restart;
                this.isInitial = isInitial;

                noSignalProcessor = NoSignalProcessor().GetEnumerator();
                UpdateNoSignal();
            }

            Action Restart;

            static public PlaybackStatistics Start(Action Restart, bool isInitial) {
                return new PlaybackStatistics(Restart, isInitial);
            }

            public const long noSignalDelay = 200;
            public const long noSignalTimeout = 5000;
            public const long noSignalTimeoutInitial = 5000;
            public const long noSignalRestartTimout = 10000;

            public bool isInitial { get; private set; }
            public bool isNoSignal { get; private set; }
            public byte signal { get; private set; }
            public double avgRenderingFps { get; private set; }

            CircularBuffer<long> renderTimes = new CircularBuffer<long>(128);
            CircularBuffer<long> decodeTimes = new CircularBuffer<long>(128);
            IEnumerator<bool> noSignalProcessor;

            private static double SecondsFromTicks(long ticks) {
                return (double)ticks / (double)Stopwatch.Frequency;
            }

            private void UpdateNoSignal() {
                noSignalProcessor.MoveNext();
                isNoSignal = noSignalProcessor.Current;
            }

            /// <summary>state machine for no signal</summary>
            /// <returns>true if no signal detected</returns>
            private IEnumerable<bool> NoSignalProcessor() {
                var timer = Stopwatch.StartNew();
                while (true) {
                    if (signal != 0) {
                        decodeTimes.Enqueue(Stopwatch.GetTimestamp());
                        isNoSignal = false;
                        timer.Restart();
                    } else {
                        if (timer.ElapsedMilliseconds > noSignalTimeout) {
                            isNoSignal = true;
                            //ConsoleLine.VideoOut("isNoSignal = true", channel);
                            if (timer.ElapsedMilliseconds > noSignalRestartTimout) {
                                if (Restart != null) {
                                    timer.Restart();
                                    Restart();
                                }
                            }
                        }
                    }
                    yield return isNoSignal;
                }
            }

            public void Update(VideoBuffer videoBuffer) {
                //update rendering times history
                renderTimes.Enqueue(Stopwatch.GetTimestamp());

                //evaluate averange rendering fps
                avgRenderingFps = renderTimes.length / SecondsFromTicks(renderTimes.last - renderTimes.first);

                //update no signal indicator
                using (var md = videoBuffer.Lock()) {
                    signal = md.value.signal;
                    md.value.signal = 0;
                }
                UpdateNoSignal();

            }
        }
    }
}
